/*
 * Copyright (C) 2006 - 2012  Campanoni Simone
 *
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
/**
 * @file system_initializer.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <metadata_manager.h>
#include <section_manager.h>
#include <compiler_memory_manager.h>
#include <platform_API.h>
#include <ir_optimization_interface.h>
#include <ir_optimization_levels_interface.h>
#include <plugin_manager.h>

// My header
#include <clr_interface.h>
#include <garbage_collector_interactions.h>
#include <iljit_plugin_manager.h>
#include <lib_lock_os.h>
#include <iljit.h>
#include <system_manager.h>
#include <jitsystem.h>
#include <system_initializer.h>
#include <general_tools.h>
#include <exception_manager.h>
// End

static inline void internal_clean_code_cache (void);
static inline void print_usage (FILE * stream);
static inline JITINT16 decode_arguments (t_system *system, JITNINT argc, char **argv);
static inline void open_dump_files (t_system *system);
static inline void print_decoders (t_system *system);
static inline void print_clrs (t_system *system);
static inline void print_codetools (t_system *system);
static inline void print_optimizationleveltools (t_system *system);
static inline void print_codetoolnames (t_system *system);
static inline void print_all_codetool_names (FILE *stream, t_system *system);
static inline void print_decoder (void *plugin);
static inline void print_clr (void *plugin);
static inline void print_garbage_collectors (t_system *system);
static inline void print_garbage_collector (void *plugin);
static inline void internal_add_opt (optimizations_t *opts, JITINT8 *optToAdd);

extern t_system *ildjitSystem;

JITINT32 system_initializer (t_system * system, JITNINT argc, char **argv) {

    /* Assertions							*/
    assert(system != NULL);
    assert(argv != NULL);

    /* Decode the command line arguments				*/
    if (decode_arguments(system, argc, argv) != 0) {
        return -1;
    }

    /* SET-UP the system static variable                            */
    ildjitSystem = system;

    /* Return							*/
    return 0;
}

static inline JITINT16 decode_arguments (t_system *system, JITNINT argc, char **argv) {
    JITNINT next_option;
    JITUINT32 count;
    JITNINT iljit_argc;
    char	*tmpPointer;
    char	*baseName;
    char buf[DIM_BUF];
    char short_options[] = "htf:p:vVNO:e:b:XH:L:P:jdAaxwcDCG:z:F:RIMmsTil";
    struct utsname platformInfo;
    const struct option long_options[] = {
        { "help",					  0,		NULL,	  'h' },
        { "trace",					  0,		NULL,	  't' },
        { "trace-options",				  1,		NULL,	  'f' },
        { "profiler",					  1,		NULL,	  'p' },
        { "verbose",					  0,		NULL,	  'v' },
        { "version",					  0,		NULL,	  'V' },
        { "no-explicit-gc",				  0,		NULL,	  'N' },
        { "optimization-level",				  1,		NULL,	  'O' },
        { "enable-optimizations",			  1,		NULL,	  'e' },
        { "disable-optimizations",			  1,		NULL,	  'b' },
        { "debug-execution",				  0,		NULL,	  'X' },
        { "heap-size",					  1,		NULL,	  'H' },
        { "linker",					  1,		NULL,	  'L' },
        { "pgc",					  1,		NULL,	  'P' },
        { "jit",					  0,		NULL,	  'j' },
        { "dla",					  0,		NULL,	  'd' },
        { "aot",					  0,		NULL,	  'A' },
        { "static",					  0,		NULL,	  'a' },
        { "precompilation-only",			  0,		NULL,	  'x' },
        { "clean-code-cache",				  0,		NULL,	  'w' },
        { "disable-linker",				  0,		NULL,	  'c' },
        { "dump-all",					  0,		NULL,	  'D' },
        { "dump-config",				  0,		NULL,	  'C' },
        { "gc",						  1,		NULL,	  'G' },
        { "optimizationleveltool",			  1,		NULL,	  'z' },
        { "oprefix",					  1,		NULL,	  'F' },
        { "disable-runtime-checks",			  0,		NULL,	  'R' },
        { "enable-ir-checks",				  0,		NULL,	  'I' },
        { "enable-machine-dependent-optimizations",	  0,		NULL,	  'M' },
        { "disable-machinecode-generation",		  0,		NULL,	  'm' },
        { "disable-execution",				  0,		NULL,	  's' },
        { "do-not-overlap-compiler-and-executor-threads", 0,		NULL,	  'T' },
        { "disable-static-constructors", 		  0,		NULL,	  'i' },
        { "optimizations", 				  0,		NULL,	  'l' },
        { NULL,						  0,		NULL,	  0   }
    };

    memset(&(system->IRVM).behavior, 0, sizeof(ir_system_behavior_t));
    (system->DLA).probabilityBoundary = DEFAULT_LAB;
    (system->IRVM).behavior.jit = JITTRUE;
    (system->IRVM).behavior.outputPrefix = strdup("ILDJIT");
    (system->cliManager).CLR.runtimeChecks = JITTRUE;
    (system->program).enableMachineCodeGeneration = JITTRUE;
    iljit_argc = argc;
    for (count = 1; count < argc; count++) {
        if (argv[count][0] != '-') {
            iljit_argc = count;
            break;
        }
    }
    count = 0;
    while (1) {
        count++;
        next_option = PLATFORM_getOptArg(iljit_argc, argv, short_options, long_options, NULL);
        if (next_option == -1) {
            break;
        }
        switch (next_option) {
            case 'h':
                print_usage(stdout);
                return -1;
            case 'i':
                (system->IRVM).behavior.disableStaticConstructors = JITTRUE;
                break;
            case 't':
                (system->IRVM).behavior.tracer = 1;
                break;
            case 'f':
                (system->IRVM).behavior.tracerOptions = atoi(optarg);
                break;
            case 'N':
                (system->IRVM).behavior.noExplicitGarbageCollection = 1;
                break;
            case 'O':
                (system->IRVM).behavior.optimizations = atoi(optarg);
                break;
            case 'p':
                (system->IRVM).behavior.profiler = atoi(optarg);
                break;
            case 'X':
                (system->IRVM).behavior.debugExecution = 1;
                break;
            case 'H':
                (system->IRVM).behavior.heapSize = atoi(optarg);
                (system->IRVM).behavior.heapSize *= 1024;
                break;
            case 'L':
                (system->IRVM).linkerOptions    = (JITINT8 *)strdup(optarg);
                break;
            case 'P':
                (system->IRVM).behavior.pgc = atoi(optarg);
                break;
            case 'T':
                (system->program).spawnNewThreadAsMainCodeExecutor = JITTRUE;
                break;
            case 'c':
                (system->IRVM).behavior.noLinker			= JITTRUE;
                break;
            case 'w':
                internal_clean_code_cache();
                exit(0);
                break;
            case 'x':
                (system->IRVM).behavior.onlyPrecompilation	= JITTRUE;
                break;
            case 'j':
                (system->IRVM).behavior.jit = JITTRUE;
                (system->IRVM).behavior.dla = JITFALSE;
                (system->IRVM).behavior.aot = JITFALSE;
                (system->IRVM).behavior.staticCompilation = JITFALSE;
                break;
            case 'd':
                (system->IRVM).behavior.dla = JITTRUE;
                (system->IRVM).behavior.jit = JITFALSE;
                (system->IRVM).behavior.aot = JITFALSE;
                (system->IRVM).behavior.staticCompilation = JITFALSE;
                break;
            case 'A':
                (system->IRVM).behavior.aot = JITTRUE;
                (system->IRVM).behavior.jit = JITFALSE;
                (system->IRVM).behavior.dla = JITFALSE;
                (system->IRVM).behavior.staticCompilation = JITFALSE;
                break;
            case 'a':
                (system->IRVM).behavior.aot = JITFALSE;
                (system->IRVM).behavior.staticCompilation = JITTRUE;
                (system->IRVM).behavior.jit = JITFALSE;
                (system->IRVM).behavior.dla = JITFALSE;
                (system->IRVM).behavior.onlyPrecompilation	= JITTRUE;
                (system->program).disableExecution = JITTRUE;
                break;
            case 'D':
                (system->IRVM).behavior.dumpAssembly.dumpAssembly = 1;
                open_dump_files(system);
                break;
            case 'G':
                (system->IRVM).behavior.gcName = optarg;
                break;
            case 'z':
                (system->IRVM).optimizer.optLevelToolName = optarg;
                break;
            case 'F':
                (system->IRVM).behavior.outputPrefix = strdup(optarg);
                break;
            case 'e':
                internal_add_opt(&((system->IRVM).optimizer.enabledOptimizations), (JITINT8 *)optarg);
                break;
            case 'b':
                internal_add_opt(&((system->IRVM).optimizer.disabledOptimizations), (JITINT8 *)optarg);
                break;
            case 'v':
                (system->IRVM).behavior.verbose = 1;
                break;
            case 'R':
                (system->cliManager).CLR.runtimeChecks = JITFALSE;
                break;
            case 'I':
                (system->IRVM).optimizer.checkProducedIR = JITTRUE;
                break;
            case 'm':
                (system->program).enableMachineCodeGeneration = JITFALSE;
                break;
            case 's':
                (system->program).disableExecution = JITTRUE;
                (system->IRVM).behavior.disableStaticConstructors = JITTRUE;
                break;
            case 'M':
                (system->IRVM).optimizer.enableMachineDependentOptimizations = JITTRUE;
                break;
            case 'l':
                print_codetoolnames(system);
                exit(0);
            case 'V':
                printf("Version: %s\n", VERSION);
                exit(0);
            case 'C':

                /* Print the compilation framework description			*/
                printf("ILDJIT compilation framework configuration\n\n");
                printf("Copyright (C) 2005-2012  Simone Campanoni\n");
                printf("Website: http://ildjit.sourceforge.net\n");
                printf("Author: Simone Campanoni        simo.xan@gmail.com\n\n");
                printf("Framework version                                  : %s\n", VERSION);
                printf("Framework flavour                                  : Static compiler, Ahead-Of-Time compiler, Dynamic-Look-Ahead compiler, ");
                if (IRVM_isCompilationAvailable(&(ildjitSystem->IRVM))) {
                    printf("Just-In-Time compiler\n");
                } else {
                    printf("Interpreter\n");
                }
                printf("Framework compiled on                              : %s %s\n", __DATE__, __TIME__);
                printf("Framework compilation flags                        : ");
#ifdef DEBUG
                printf("DEBUG ");
#endif
#ifdef PRINTDEBUG
                printf("PRINTDEBUG ");
#endif
#ifdef PROFILE
                printf("PROFILE ");
#endif
                printf("\n");
                printf("Framework installation prefix                      : %s\n", PREFIX);

                printf("Framework threads:\n");
                printf("     Bytecode -> IR translation                 : %d\n", MAX_CILIR_LOW_THREADS + MAX_CILIR_HIGH_THREADS);
                printf("     IR -> machine code translation             : %d\n", MAX_IRMACHINECODE_LOW_THREADS + MAX_IRMACHINECODE_HIGH_THREADS);
                printf("     Dynamic-Look-Ahead compilation             : %d\n", MAX_DLA_LOW_THREADS + MAX_DLA_HIGH_THREADS);
                printf("     Code optimizations                         : ");
                if (PLATFORM_getProcessorsNumber() < (MAX_IROPTIMIZER_LOW_THREADS + MAX_IROPTIMIZER_HIGH_THREADS)) {
                    printf("%d\n", (JITINT32) PLATFORM_getProcessorsNumber());
                } else {
                    printf("%d\n", MAX_IROPTIMIZER_LOW_THREADS + MAX_IROPTIMIZER_HIGH_THREADS);
                }
                printf("     Global memories initialization             : %d\n", START_CCTOR_LOW_THREADS + START_CCTOR_HIGH_THREADS);

                /* Print the platform description		*/
                if (PLATFORM_gethostname(buf, sizeof(char) * DIM_BUF) == -1) {
                    snprintf(buf, sizeof(char) * DIM_BUF, "Unavailable");
                }
                printf("Platform Name                                   : %s\n", buf);
                if (PLATFORM_getPlatformInfo(&platformInfo) == 0) {
                    snprintf(buf, sizeof(char) * DIM_BUF, "%s %s %s", platformInfo.sysname, platformInfo.release, platformInfo.machine);
                } else {
                    snprintf(buf, sizeof(char) * DIM_BUF, "Unavailable");
                }
                printf("Platform Info                                   : %s\n", buf);
                snprintf(buf, DIM_BUF, "%s", (char*) PLATFORM_getUserName());
                printf("User                                            : %s\n", buf);

                /* Print header object information		*/
                if (GC_getHeaderSize() > HEADER_FIXED_SIZE) {
                    printf("	Warning! GC_getHeaderSize != HEADER_FIXED_SIZE\n");
                    printf("	Please contact Simone Campanoni (simo.xan@gmail.com) as soon as possible reporting this output\n");
                    printf("	HEADER_FIXED_SIZE = %d\n", HEADER_FIXED_SIZE);
                    printf("	Monitor within headers = %d\n", (JITINT32) sizeof(ILJITMonitor));
                }
                printf("\n");

                /* Print the libraries description		*/
                printf("Libmanfred\n");
                printf("           Version                              : %s\n", libmanfredVersion());
                libmanfredCompilationTime(buf, DIM_BUF);
                printf("           Compiled On                          : %s\n", buf);
                libmanfredCompilationFlags(buf, DIM_BUF);
                printf("           Compilation Flags                    : %s\n", buf);
                printf("Libdla\n");
                printf("           Version                              : %s\n", libdlaVersion());
                libdlaCompilationTime(buf, DIM_BUF);
                printf("           Compiled On                          : %s\n", buf);
                libdlaCompilationFlags(buf, DIM_BUF);
                printf("           Compilation Flags                    : %s\n", buf);
                printf("Libclimanager\n");
                printf("           Version                              : %s\n", libclimanagerVersion());
                libclimanagerCompilationTime(buf, DIM_BUF);
                printf("           Compiled On                          : %s\n", buf);
                libclimanagerCompilationFlags(buf, DIM_BUF);
                printf("           Compilation Flags                    : %s\n", buf);
                printf("Libirvirtualmachine\n");
                printf("           Version                              : %s\n", libirvirtualmachineVersion());
                libirvirtualmachineCompilationTime(buf, DIM_BUF);
                printf("           Compiled On                          : %s\n", buf);
                libirvirtualmachineCompilationFlags(buf, DIM_BUF);
                printf("           Compilation Flags                    : %s\n", buf);
                printf("Libiljitiroptimizer\n");
                printf("           Version                              : %s\n", libiljitiroptimizerVersion());
                libiljitiroptimizerCompilationTime(buf, DIM_BUF);
                printf("           Compiled On                          : %s\n", buf);
                libiljitiroptimizerCompilationFlags(buf, DIM_BUF);
                printf("           Compilation Flags                    : %s\n", buf);
                printf("Libirmanager\n");
                printf("           Version                              : %s\n", libirmanagerVersion());
                libirmanagerCompilationTime(buf, DIM_BUF);
                printf("           Compiled On                          : %s\n", buf);
                libirmanagerCompilationFlags(buf, DIM_BUF);
                printf("           Compilation Flags                    : %s\n", buf);
                printf("Libiljitmm\n");
                printf("           Version                              : %s\n", libiljitmmVersion());
                libiljitmmCompilationTime(buf, DIM_BUF);
                printf("           Compiled On                          : %s\n", buf);
                libiljitmmCompilationFlags(buf, DIM_BUF);
                printf("           Compilation Flags                    : %s\n", buf);
                printf("Libiljitsm\n");
                printf("           Version                              : %s\n", libiljitsmVersion());
                libiljitsmCompilationTime(buf, DIM_BUF);
                printf("           Compiled On                          : %s\n", buf);
                libiljitsmCompilationFlags(buf, DIM_BUF);
                printf("           Compilation Flags                    : %s\n", buf);
                printf("Libiljitu\n");
                printf("           Version                              : %s\n", libiljituVersion());
                libiljituCompilationTime(buf, DIM_BUF);
                printf("           Compiled On                          : %s\n", buf);
                libiljituCompilationFlags(buf, DIM_BUF);
                printf("           Compilation Flags                    : %s\n", buf);
                printf("Libcompilermemorymanager\n");
                printf("           Version                              : %s\n", libcompilermemorymanagerVersion());
                libcompilermemorymanagerCompilationTime(buf, DIM_BUF);
                printf("           Compiled On                          : %s\n", buf);
                libcompilermemorymanagerCompilationFlags(buf, DIM_BUF);
                printf("           Compilation Flags                    : %s\n", buf);
                printf("Libxan\n");
                printf("           Version                              : %s\n", libxanVersion());
                libxanCompilationTime(buf, DIM_BUF);
                printf("           Compiled On                          : %s\n", buf);
                libxanCompilationFlags(buf, DIM_BUF);
                printf("           Compilation Flags                    : %s\n", buf);


                /* Print the plugin informations		*/
                printf("\n");
                printf("CLR plugins                                 :\n");
                print_clrs(system);
                printf("\n");
                printf("Decoder plugins                                 :\n");
                print_decoders(system);
                printf("\n");
                printf("Garbage collectors plugins                      :\n");
                print_garbage_collectors(system);
                printf("\n");
                printf("Optimizationleveltools                          :\n");
                print_optimizationleveltools(system);
                printf("\n");
                printf("Codetools                                       :\n");
                print_codetools(system);
                exit(0);
            case '?':
                print_usage(stderr);
                return -1;
            default:
                print_err("ERROR: Given option not correct. ", 0);
                print_usage(stderr);
                abort();
        }
    }

    if (argc <= count) {
        print_usage(stderr);
        return -1;
    }
    (system->arg).argc 	= argc - count;
    (system->arg).argv 	= &(argv[count]);
    tmpPointer		= strdup((system->arg).argv[0]);
    baseName		= basename(tmpPointer);
    system->programName	= strdup(baseName);
    freeFunction(tmpPointer);

    return 0;
}

static inline void print_usage (FILE * stream) {
    fprintf(stream, "\nUsage: ildjit [options] file [bytecode_arguments]\n");
    fprintf(stream, "Options:\n");
    fprintf(stream, "   -h    --help                                           Display this usage information\n");
    fprintf(stream, "   -t    --trace                                          To enable the tracer of the method executed and to print their name\n");
    fprintf(stream, "   -f    --trace-options=num                              Options of the method tracer.\n");
    fprintf(stream, "                                                                 0: trace all methods\n");
    fprintf(stream, "                                                                 1: trace only methods that do not belong to any library\n");
    fprintf(stream, "                                                                 2: trace only methods that belong to libraries\n");
    fprintf(stream, "                                                                 3: trace only methods provided by CLR\n");
    fprintf(stream, "   -p    --profiler=num                                   To enable the profiler and to print to stderr the profiling informations after the execution with the deep information level equal to num\n");
    fprintf(stream, "   -v    --verbose                                        To ability a verbose output\n");
    fprintf(stream, "   -V    --version                                        To print out the version and exit.\n");
    fprintf(stream, "   -N    --no-explicit-gc                                 Not collect the memory garbage explicitly\n");
    fprintf(stream, "   -O    --optimization-level=level                       Set the optimization level(e.g. -O0, -O1); -O0 means no optimizations\n");
    fprintf(stream, "   -e    --enable-optimizations=LIST                      Enable the specified optimizations.\n");
    fprintf(stream, "                                                          LIST is a comma separated list of optimizations.\n");
    fprintf(stream, "   -b    --disable-optimizations=LIST                     Disable the specified optimizations.\n");
    fprintf(stream, "                                                          LIST is a comma separated list of optimizations.\n");
    fprintf(stream, "   -X    --debug-execution                                Translate and execute each CIL method with extra checkers\n");
    fprintf(stream, "   -H    --heap-size=num                                  Set the size of the heap to num kilobytes\n");
    fprintf(stream, "   -L    --lab=num                                        Set the initial Look-Ahead Probability of the DLA technique\n");
    fprintf(stream, "   -P    --pgc=num                                        Active Profile Guided Compilation\n");
    fprintf(stream, "   -j    --jit                                            Enable JIT technique\n");
    fprintf(stream, "   -d    --dla                                            Enable DLA technique\n");
    fprintf(stream, "   -A    --aot                                            Enable AOT technique\n");
    fprintf(stream, "   -a    --static                                         Enable the static compilation scheme\n");
    fprintf(stream, "   -x    --precompilation-only                            Enable only compilation performed before starting the execution of the entry point of the program");
    fprintf(stream, "   -w    --clean-code-cache                               Clean the code cache completely\n");
    fprintf(stream, "   -c    --disable-linker                                 Compile or assemble the program, but do not link. The ultimate output is in the form of an object file.\n");
    fprintf(stream, "   -D    --dump-all                                       Dump all the CIL methods translated in the CIL, IR and assembly languages\n");
    fprintf(stream, "   -C    --dump-config                                    Dump configuration information for the ILDJIT engine\n");
    fprintf(stream, "   -G    --gc=name                                        Use the garbage collector name\n");
    fprintf(stream, "   -z    --optimizationleveltool=name                     Use the optimizationleveltool called \"name\"\n");
    fprintf(stream, "   -F    --oprefix=name                                   Set to name the prefix for each files generated by the compiler\n");
    fprintf(stream, "   -R    --disable-runtime-checks                         Disable checks performed at runtime. Notice that the code produced is not CLI-compliant any more.\n");
    fprintf(stream, "   -I    --enable-ir-checks                               Enable checks for IR code produced by the compiler\n");
    fprintf(stream, "   -M    --enable-machine-dependent-optimizations         Enable code optimizations that are specific of the underlying machine\n");
    fprintf(stream, "   -m    --disable-machinecode-generation                 Do not generate machine code\n");
    fprintf(stream, "   -s    --disable-execution                              Disable the execution of the produced code\n");
    fprintf(stream, "   -T    --do-not-overlap-compiler-and-executor-threads   Do not use compiler threads to execute code and vice versa\n");
    fprintf(stream, "   -i    --disable-static-constructors                    Disable both code generation and execution of static constructors\n");
    fprintf(stream, "   -l    --optimizations                                  Dump the optimizations (codetools) available inside the paths specified by ILDJIT_PLGUINS\n");
    if (stream == stderr) {
        exit(1);
    }
    exit(0);
}

static inline void print_codetoolname (void *plugin) {
    JITUINT64	jobKind;

    /* Fetch the plugin		*/
    ir_optimization_interface_t *plugin_handle = (ir_optimization_interface_t *) plugin;
    assert(plugin_handle != NULL);
    jobKind		= plugin_handle->get_job_kind();

    printf("		%s    :  %s\n", IROPTIMIZER_jobToString(jobKind), IROPTIMIZER_jobToShortName(jobKind));

    return ;
}

static inline void print_codetool (void *plugin) {
    char buf[DIM_BUF];

    /* Fetch the plugin		*/
    ir_optimization_interface_t *plugin_handle = (ir_optimization_interface_t *) plugin;
    assert(plugin_handle != NULL);

    /* Print the plugin		*/
    printf("	Codetool = %s\n", IROPTIMIZER_jobToString(plugin_handle->get_job_kind()));
    printf("		Version			= %s\n", plugin_handle->get_version());
    printf("		Author			= %s\n", plugin_handle->get_author());
    printf("		Description		= %s\n", plugin_handle->get_information());
    if (plugin_handle->getCompilationFlags != NULL) {
        plugin_handle->getCompilationFlags(buf, DIM_BUF);
        printf("		Compilation flags	= %s\n", buf);
    }
    if (plugin_handle->getCompilationTime != NULL) {
        plugin_handle->getCompilationTime(buf, DIM_BUF);
        printf("		Compilation on		= %s\n", buf);
    }

    return ;
}

static inline void print_optimizationleveltool (void *plugin) {
    char buf[DIM_BUF];
    ir_optimization_levels_interface_t	*plugin_handle;

    /* Fetch the plugin		*/
    plugin_handle = (ir_optimization_levels_interface_t *) plugin;
    assert(plugin_handle != NULL);

    /* Print the plugin		*/
    printf("	Optimizationleveltool = %s\n", plugin_handle->getName());
    printf("		Version			= %s\n", plugin_handle->getVersion());
    printf("		Author			= %s\n", plugin_handle->getAuthor());
    printf("		Description		= %s\n", plugin_handle->getInformation());
    if (plugin_handle->getCompilationFlags != NULL) {
        plugin_handle->getCompilationFlags(buf, DIM_BUF);
        printf("		Compilation flags	= %s\n", buf);
    }
    if (plugin_handle->getCompilationTime != NULL) {
        plugin_handle->getCompilationTime(buf, DIM_BUF);
        printf("		Compilation on		= %s\n", buf);
    }

    return ;
}

static inline void print_all_codetool_names (FILE *stream, t_system *system) {
    JITUINT64	temp;

    temp	= 0x1;
    while (JITTRUE) {
        JITINT8	*name;
        name	 = (JITINT8 *)IROPTIMIZER_jobToString(temp);
        if (name == NULL) {
            break;
        }
        fprintf(stream, "                 %s:   %s\n", name, IROPTIMIZER_jobToShortName(temp));
        temp	= temp << 1;
    }

    return ;
}

static inline void print_codetoolnames (t_system *system) {
    JITINT8	*path;

    /* Fetch the path			*/
    path	= IROPTIMIZER_getCodetoolPaths();

    /* Print all the possible codetools	*/
    printf("The following codetools can be installed in the system\n");
    print_all_codetool_names(stdout, system);
    printf("\n");

    /* Print the codetools		*/
    printf("The following codetools are available in the system\n");
    PLUGIN_iteratePlugins(path, print_codetoolname, (JITINT8 *)"plugin_interface");

    return ;
}

static inline void print_codetools (t_system *system) {
    JITINT8	*path;

    /* Fetch the path		*/
    path	= IROPTIMIZER_getCodetoolPaths();

    /* Print the codetools		*/
    PLUGIN_iteratePlugins(path, print_codetool, (JITINT8 *)"plugin_interface");

    return ;
}

static inline void print_optimizationleveltools (t_system *system) {
    JITINT8	*path;

    /* Fetch the path		*/
    path	= IROPTIMIZER_getOptimizationleveltoolPaths();

    /* Print the codetools		*/
    PLUGIN_iteratePlugins(path, print_optimizationleveltool, (JITINT8 *)"plugin_interface");

    return ;
}

static inline void print_clrs (t_system *system) {
    PLUGIN_iteratePlugins((JITINT8 *)get_path_clr_plugin(), print_clr, (JITINT8 *)"plugin_interface");

    return ;
}

static inline void print_decoders (t_system *system) {
    PLUGIN_iteratePlugins((JITINT8 *)PATH_PLUGIN, print_decoder, (JITINT8 *)"plugin_interface");

    return ;
}

static inline void print_clr (void *plugin) {
    char buf[DIM_BUF];

    clr_interface_t *plugin_handle = (clr_interface_t *) plugin;
    assert(plugin_handle != NULL);
    printf("	Name	= %s\n", plugin_handle->getName());
    printf("		Version			= %s\n", plugin_handle->getVersion());
    printf("		Author			= %s\n", plugin_handle->getAuthor());
    plugin_handle->getCompilationFlags(buf, DIM_BUF);
    printf("		Compilation flags	= %s\n", buf);
    plugin_handle->getCompilationTime(buf, DIM_BUF);
    printf("		Compilation on		= %s\n", buf);

    return ;
}

static inline void print_decoder (void *plugin) {
    char buf[DIM_BUF];
    t_plugin_interface *plugin_handle = (t_plugin_interface *) plugin;

    assert(plugin_handle != NULL);
    printf("	Name	= %s\n", plugin_handle->getName());
    printf("		Version			= %s\n", plugin_handle->getVersion());
    printf("		Author			= %s\n", plugin_handle->getAuthor());
    plugin_handle->getCompilationFlags(buf, DIM_BUF);
    printf("		Compilation flags	= %s\n", buf);
    plugin_handle->getCompilationTime(buf, DIM_BUF);
    printf("		Compilation on		= %s\n", buf);
}

static inline void print_garbage_collectors (t_system *system) {
    PLUGIN_iteratePlugins((JITINT8 *)PATH_GARBAGE_COLLECTOR_PLUGIN, print_garbage_collector, (JITINT8 *)"plugin_interface");
}

static inline void print_garbage_collector (void *plugin) {
    char buf[DIM_BUF];
    t_garbage_collector_plugin *plugin_handle = (t_garbage_collector_plugin *) plugin;

    assert(plugin_handle != NULL);
    printf("	Name	= %s\n", plugin_handle->getName());
    printf("		Version			= %s\n", plugin_handle->getVersion());
    printf("		Author			= %s\n", plugin_handle->getAuthor());
    plugin_handle->getCompilationFlags(buf, DIM_BUF);
    printf("		Compilation flags	= %s\n", buf);
    plugin_handle->getCompilationTime(buf, DIM_BUF);
    printf("		Compilation on		= %s\n", buf);
}

static inline void open_dump_files (t_system *system) {
    char buf[DIM_BUF];

    /* Assertions			        */
    assert(system != NULL);
    assert((system->IRVM).behavior.outputPrefix != NULL);

    /* Open instructions files              */
    snprintf(buf, sizeof(char) * DIM_BUF, "%s.IR", (system->IRVM).behavior.outputPrefix);
    (system->IRVM).behavior.dumpAssembly.dumpFileIR = fopen(buf, "w");
    if ((system->IRVM).behavior.dumpAssembly.dumpFileIR == NULL) {
        snprintf(buf, sizeof(char) * DIM_BUF, "ERROR = Cannot open file %s to wrte the assembly. ", "IR");
        print_err(buf, errno);
        abort();
    }
    snprintf(buf, sizeof(char) * DIM_BUF, "%s.Libjit", (system->IRVM).behavior.outputPrefix);
    (system->IRVM).behavior.dumpAssembly.dumpFileJIT = fopen(buf, "w");
    if ((system->IRVM).behavior.dumpAssembly.dumpFileJIT == NULL) {
        snprintf(buf, sizeof(char) * DIM_BUF, "ERROR = Cannot open file %s to wrte the assembly. ", "Libjit");
        print_err(buf, errno);
        abort();
    }
    snprintf(buf, sizeof(char) * DIM_BUF, "%s.Assembly", (system->IRVM).behavior.outputPrefix);
    (system->IRVM).behavior.dumpAssembly.dumpFileAssembly = fopen(buf, "w");
    if ((system->IRVM).behavior.dumpAssembly.dumpFileAssembly == NULL) {
        snprintf(buf, sizeof(char) * DIM_BUF, "ERROR = Cannot open file %s to wrte the assembly. ", "Assembly");
        print_err(buf, errno);
        abort();
    }

    return;
}

static inline void internal_clean_code_cache (void) {
    JITINT8 *homedir;
    JITINT8 *cachedir;
    JITINT8 *buf;
    JITUINT32 bufLength;

    /* Fetch the name of the cache		*/
    cachedir = (JITINT8 *) "/.ildjit";

    /* Fetch the home directory		*/
    homedir = PLATFORM_getUserDir();

    /* Allocate the necessary memory	*/
    bufLength = STRLEN(homedir) + STRLEN(cachedir) + 1;
    buf = allocFunction(bufLength);

    /* Create the name of the directory of	*
     * the cache				*/
    SNPRINTF(buf, bufLength, "%s%s", homedir, cachedir);

    /* Remove the directory			*/
    PLATFORM_rmdir_recursively(buf);

    /* Free the memory			*/
    freeFunction(buf);

    /* Return				*/
    return;
}

static inline void internal_add_opt (optimizations_t *opts, JITINT8 *optToAdd) {
    JITINT8	*oldBuf;

    if (opts->optimizationsList == NULL) {
        opts->optimizationsListSize		= STRLEN(optToAdd) + 1;
        opts->optimizationsList 		= (JITINT8 *) strdup((char *)optToAdd);
        return ;
    }

    oldBuf				= allocFunction(opts->optimizationsListSize);
    opts->optimizationsListSize	+= (strlen(optarg) + 1);
    SNPRINTF(oldBuf, opts->optimizationsListSize, "%s", opts->optimizationsList);
    opts->optimizationsList 	= dynamicReallocFunction(opts->optimizationsList, opts->optimizationsListSize);
    SNPRINTF(opts->optimizationsList, opts->optimizationsListSize, "%s,%s", oldBuf, optarg);
    freeMemory(oldBuf);

    return ;
}
