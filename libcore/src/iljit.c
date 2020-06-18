/*
 * Copyright (C) 2006 - 2012 Campanoni Simone
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <time.h>
#include <ir_virtual_machine.h>
#include <platform_API.h>
#include <ildjit.h>
#include <jitsystem.h>
#include <iljit-utils.h>

// My headers
#include <iljit.h>
#include <config.h>
#include <system_manager.h>
#include <loader.h>
#include <error_codes.h>
#include <cil_ir_translator.h>
#include <general_tools.h>
#include <executor.h>
#include <ildjit_profiler.h>
#include <runtime.h>
// End

#define FIFO_DIRECTORY		"/tmp/ildjit-generator"
#define FIFO_GENERATOR		"generator"

typedef struct {
    pid_t	instance_pid;
    pid_t	loader_pid;
} wait_data_t;

/**
 * @brief Call the loader
 *
 * Call the loader and control the return value, if it reppresent an error, it print the associated error string. The error code is specified by the error_codes.h file.
 */
JITBOOLEAN	call_loader	(t_system *system);

JITINT32	start_generator	(t_system *system);
void *		waitInstance	(void* arg);
JITINT32 iljit_main (t_system *system, JITINT32 argc, char **argv, JITBOOLEAN multiApps);

char		prefix				[256];
char		library_path			[256];
char		path_plugin			[256];
char		path_garbage_collector_plugin	[256];

JITINT32 main_singleapp (JITINT32 argc, char **argv) {
    t_system	*system;

    /* Initialize the variables		*/
    system	= NULL;

    return iljit_main(system, argc, argv, JITFALSE);
}

JITINT32 main_multiapps (JITINT32 argc, char **argv) {
    t_system	*system;

    /* Bootstrap the generator		*/
    PDEBUG("ILJIT: Bootstrap of the generator\n");
    system  = generator_bootstrapper(argc, argv);
    assert(system != NULL);

    /* Start the generator			*/
    PDEBUG("ILJIT: Starting the generator\n");
    if (start_generator(system) != 0) {
        return EXIT_FAILURE;
    }

    /* Return				*/
    return EXIT_SUCCESS;
}

JITINT32 ILDJIT_main (JITINT32 argc, char **argv, JITBOOLEAN multiapp) {
    JITINT32 	returnValue;

    /* Initialize the paths			*/
    init_paths();

    /* Run the program			*/
    if (multiapp) {
        returnValue	= main_multiapps(argc, argv);
    } else {
        returnValue	= main_singleapp(argc, argv);
    }

    /* Return				*/
    return returnValue;
}

JITINT32 iljit_main (t_system *system, JITINT32 argc, char **argv, JITBOOLEAN multiApps) {
    ProfileTime	startTime;
    JITINT32 	result;

    /* Fetch the start time                 */
    profilerStartTime(&startTime);

    if (multiApps) {

        /* Bootstrap current instance		*/
        system_bootstrapper_multiapp(system, argc, argv);
    } else {

        /* Bootstrap the system			*/
        system = system_bootstrapper(argc, argv);
    }
    assert(system != NULL);

    /* Load the CIL program in memory	*/
    PDEBUG("ILJIT: Dispatch the load binary job to the right plugin\n");
    if (call_loader(system) != 0) {
        return -1;
    }

    /* Store the startTime into the profiler*/
    memcpy(&((system->profiler).startTime), &startTime, sizeof(ProfileTime));

    /* Start the executor			*/
    PDEBUG("ILJIT: Start the executor for the %s bytecode\n", argv[1]);
    startExecutor(system);
    PDEBUG("ILJIT: Result = %d\n", (system->program).result);

    /* Shutdown				*/
    result = (system->program).result;
    PDEBUG("ILJIT: Shutdown the system\n");
    ILDJIT_quit(result);

    return result;
}

JITBOOLEAN call_loader (t_system *system) {
    JITINT16 error;
    char buf[1024];
    ProfileTime startTime;

    /* Profiling            */
    if ((system->IRVM).behavior.profiler >= 1) {
        /* Store the start time    */
        profilerStartTime(&startTime);
    }

    /* Assertions  */
    assert(system != NULL);

    if ((error = loader(system)) != 0) {
        switch (error) {
            case NO_DECODER_FOUND:
                print_err("LOADER: No decoder useful found. ", 0);
                break;
            case NO_READ_POSSIBLE:
                print_err("LOADER: ERROR= No read was possible from the file. ", errno);
                break;
            case NO_SEEK_POSSIBLE:
                print_err("LOADER: ERROR= No seek was possible from the file. ", errno);
                break;
            case DECODER_RETURN_AN_ERROR:
                fprintf(stdout, "Uncaught exception: %s.%s : the format of the file %s is invalid\n", "System", "BadImageFormatException", (system->cliManager).entryPoint.binary->name);
                break;
            case ENTRY_POINT_NOT_VALID:
                fprintf(stdout, "Uncaught exception: %s.%s : %s\n", "System", "BadImageFormatException", "invalid entrypoint found");
                break;
            case CANNOT_DECODE_ASSEMBLIES_REFERENCED:
                fprintf(stdout, "Uncaught exception: %s.%s : %s\n", "System", "BadImageFormatException", "cannot decode assemblies");
                break;
            default:
                snprintf(buf, sizeof(buf), "ILJIT: ERROR: The loader return the error not known %d. ", error);
                print_err(buf, 0);
        }
        ILDJIT_quit(-1);
        abort();
    }

    /* Profiling            */
    if ((system->IRVM).behavior.profiler >= 1) {
        JITFLOAT32 wallTime;

        /* Store the stop time        */
        (system->profiler).cil_loading_time += profilerGetTimeElapsed(&startTime, &wallTime);
        (system->profiler).wallTime.cil_loading_time += wallTime;
    }

    return 0;
}

JITINT32 start_generator(t_system *system) {
    FILE		*generator_fifo;
    char		generator_fifo_path	[PATH_MAX];

    /* Prepare the fifo directory		*/
    if ( (PLATFORM_mkdir(FIFO_DIRECTORY, 0777) == -1) && (errno != EEXIST) ) {
        perror("mkdir");
        exit(EXIT_FAILURE);
    }

    /* Prepare the generator fifo		*/
    snprintf(generator_fifo_path, PATH_MAX, "%s/%s", FIFO_DIRECTORY, FIFO_GENERATOR);
    if ( (PLATFORM_mkfifo(generator_fifo_path, 0777) == -1) && (errno != EEXIST) ) {
        perror("PLATFORM_mkfifo");
        return -1;
    }
    if ( (generator_fifo = fopen(generator_fifo_path, "r+")) == NULL ) {
        perror("fopen");
        return -1;
    }

    /* Start the wrapper loop		*/
    do {
        char		loader_fifo_path[PATH_MAX];
        FILE		*loader_fifo;
        wait_data_t	*wait_data;
        pthread_t	wait_thread;

        wait_data = malloc(sizeof(wait_data_t));
        if (fscanf(generator_fifo, "%d", &(wait_data->loader_pid)) == EOF) {
            abort();
        }
        PDEBUG("GENERATOR: Arrived request from process %d\n", wait_data->loader_pid);

        /* Create a FIFO for the loader		*/
        snprintf(loader_fifo_path, PATH_MAX, "%s/%d", FIFO_DIRECTORY, wait_data->loader_pid);
        if ( (loader_fifo = fopen(loader_fifo_path, "a+b")) == NULL ) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        /* Generate the instance running ILDJIT	*/
        wait_data->instance_pid = PLATFORM_fork();
        if ( wait_data->instance_pid == 0 ) {
            char	environ_path[PATH_MAX];
            char	cmdline_path[PATH_MAX];
            char	stdin_path[PATH_MAX];
            char	stdout_path[PATH_MAX];
            char	stderr_path[PATH_MAX];
            char	status_path[PATH_MAX];
            FILE	*environ_file;
            FILE	*cmdline_file;
            FILE	*status_file;
            int	stdin_fd;
            int	stdout_fd;
            int	stderr_fd;
            char	*var_name;
            char	*var_value;
            char	*arg;
            char	**instance_argv;
            int	instance_argc;
            size_t	len;

            PDEBUG("INSTANCE: pid = %d\n", getpid());

            /* Get the loader environment				*/
            snprintf(environ_path, PATH_MAX, "/proc/%d/environ", wait_data->loader_pid);
            if ( (environ_file = fopen(environ_path, "r")) == NULL ) {
                perror("fopen");
                exit(EXIT_FAILURE);
            }

            /* Clear the old environment				*/
            PLATFORM_clearenv();

            /* Set the current environment to match the loader one	*/
            var_name = NULL;
            while( PLATFORM_getdelim(&var_name, &len, '\0', environ_file) != -1 ) {
                var_value = strchr(var_name, '=') + 1;
                *(strchr(var_name, '=')) = '\0';
                PLATFORM_setenv(var_name, var_value, 1);
            }
            free(var_name);
            fclose(environ_file);

#ifdef DEBUG
            /* Display the new environment				*/
            PDEBUG("Environment is:\n");
            char **var;
            for ( var = environ; *var != NULL; var++ ) {
                PDEBUG("%s\n", *var);
            }
#endif

            /* Set the loader working directory			*/
            if (chdir(getenv("PWD")) != 0) {
                abort();
            }

            /* Set the command line					*/
            snprintf(cmdline_path, PATH_MAX, "/proc/%d/cmdline", wait_data->loader_pid);
            if ( (cmdline_file = fopen(cmdline_path, "rb")) == NULL ) {
                perror("fopen");
                exit(EXIT_FAILURE);
            }
            // TODO fix
#define MAX_ARGS 25
            instance_argv = malloc(MAX_ARGS * sizeof(char*));
            instance_argc = 0;
            arg = NULL;
            while( PLATFORM_getdelim(&arg, &len, '\0', cmdline_file) != -1 ) {
                fprintf(stderr, "INSTANCE: argv[%d] = %s\n", instance_argc, arg);
                instance_argv[instance_argc] = malloc(strlen(arg) + 1);
                strncpy(instance_argv[instance_argc++], arg, strlen(arg) + 1);
            }
            free(arg);
            fclose(cmdline_file);

            /* Adjust file descriptors				*/
            PDEBUG("INSTANCE: Adjusting file descriptors\n");
            snprintf(stdin_path,	PATH_MAX,		"/proc/%d/fd/0", wait_data->loader_pid);
            snprintf(stdout_path,	PATH_MAX,		"/proc/%d/fd/1", wait_data->loader_pid);
            snprintf(stderr_path,	PATH_MAX,		"/proc/%d/fd/2", wait_data->loader_pid);
            stdin_fd		= open(stdin_path,	O_RDONLY);
            stdout_fd		= open(stdout_path,	O_WRONLY);
            stderr_fd		= open(stderr_path,	O_WRONLY);
            dup2(stdin_fd,		STDIN_FILENO);
            dup2(stdout_fd,		STDOUT_FILENO);
            dup2(stderr_fd,		STDERR_FILENO);
            close(stdin_fd);
            close(stdout_fd);
            close(stderr_fd);
            PDEBUG("INSTANCE: File descriptors adjusted\n");

            /* Adjust real, effective, saved set and file system	*/
            /* UID and GID						*/
            PDEBUG("INSTANCE: Adjusting real, effective, saved set and file system UID and GID\n");
            snprintf(status_path, PATH_MAX, "/proc/%d/status", wait_data->loader_pid);
            if ( (status_file = fopen(status_path, "rb")) == NULL ) {
                perror("fopen");
                exit(EXIT_FAILURE);
            }
            var_name = NULL;
            int uid0, uid1, uid2, uid3;
            int gid0, gid1, gid2, gid3;
            while( PLATFORM_getline(&var_name, &len, status_file) != -1 ) {
                if ( strstr(var_name, "Uid") == var_name ) {
                    sscanf(var_name, "Uid:\t%d\t%d\t%d\t%d", &uid0, &uid1, &uid2, &uid3);
                    PDEBUG("%s", var_name);
                    PDEBUG("uid0 = %d\n", uid0);
                    PDEBUG("uid1 = %d\n", uid1);
                    PDEBUG("uid2 = %d\n", uid2);
                    PDEBUG("uid3 = %d\n", uid3);
                }
                if ( strstr(var_name, "Gid") == var_name ) {
                    sscanf(var_name, "Gid:\t%d\t%d\t%d\t%d", &gid0, &gid1, &gid2, &gid3);
                    PDEBUG("%s", var_name);
                    PDEBUG("gid0 = %d\n", gid0);
                    PDEBUG("gid1 = %d\n", gid1);
                    PDEBUG("gid2 = %d\n", gid2);
                    PDEBUG("gid3 = %d\n", gid3);
                }
            }
            free(var_name);
            fclose(status_file);
            PLATFORM_setgid(gid0);
            PLATFORM_setuid(uid0);

            /* Tell the loader the instance pid			*/
            fprintf(loader_fifo, "%d\n", getpid());
            fclose(loader_fifo);

            /* Start the stuff					*/
            return iljit_main(system, instance_argc, instance_argv, JITTRUE);
        }

        /* Create a thread to wait for the instance to end	*/
        pthread_create(&wait_thread, NULL, waitInstance, (void*) wait_data);

    } while( 1 );

    return EXIT_SUCCESS;
}

void * waitInstance(void* arg) {
    wait_data_t	*wait_data;
    int		status;
    char		loader_fifo_path[PATH_MAX];
    FILE		*loader_fifo;

    /* Fetch the wait data				*/
    wait_data = (wait_data_t*) arg;

    /* Wait for the required instance to terminate	*/
    PLATFORM_waitpid(wait_data->instance_pid, &status, 0);

    /* Check exit status				*/
    if ( PLATFORM_WIFEXITED(status) ) {
        fprintf(stderr, "GENERATOR: Instance %d exit status: %d\n", wait_data->instance_pid, PLATFORM_WEXITSTATUS(status));
    } else {
        fprintf(stderr, "GENERATOR: Instance %d exit anormally: --%d--\n", wait_data->instance_pid, status);
    }

    /* Tell the loader about the return value		*/
    snprintf(loader_fifo_path, PATH_MAX, "%s/%d", FIFO_DIRECTORY, wait_data->loader_pid);
    if ( (loader_fifo = fopen(loader_fifo_path, "a+b")) == NULL ) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fprintf(loader_fifo, "%d\n", status);
    fclose(loader_fifo);

    /* Free the allocated data			*/
    free(wait_data);

    return NULL;
}
