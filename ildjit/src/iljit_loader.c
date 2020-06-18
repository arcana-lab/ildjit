/*
 * Copyright (C) 2012 Campanoni Simone
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
#include <linux/limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FIFO_DIRECTORY		"/tmp/ildjit-generator"
#define FIFO_GENERATOR		"generator"

void	signal_sender		(int signal_number);

pid_t	instance_pid;

int main() {
    int			status;
    char			generator_fifo_path	[PATH_MAX];
    char			loader_fifo_path	[PATH_MAX];
    FILE			*generator_fifo;
    FILE			*loader_fifo;
    struct sigaction	sa;
    int			i;

    /* Prepare the path for required files		*/
    snprintf(loader_fifo_path,	PATH_MAX, "%s/%d",	FIFO_DIRECTORY, getpid());
    snprintf(generator_fifo_path,	PATH_MAX, "%s/%s",	FIFO_DIRECTORY, FIFO_GENERATOR);

    /* Create a FIFO for this loader		*/
    if ( mkfifo(loader_fifo_path, 0777) == -1 ) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    /* Open the created FIFO			*/
    if ( (loader_fifo = fopen(loader_fifo_path, "r+b")) == NULL ) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    /* Open the generator FIFO			*/
    if ( (generator_fifo = fopen(generator_fifo_path, "a+b")) == NULL ) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    /* Send the generator the pid of this loader	*/
    fprintf(generator_fifo, "%d\n", getpid());
    fflush(generator_fifo);

    /* Read the instace pid				*/
    fscanf(loader_fifo, "%d", &instance_pid);

    /* Redirect signals (handlable ones)		*/
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_sender;
    for( i = 1; i <= 30; i++ ) {
        sigaction(i, &sa, NULL);
    }

    /* Close my file descriptors			*/
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    /* Read the instance exit status		*/
    fscanf(loader_fifo, "%d", &status);

    /* Cleanup					*/
    fclose(generator_fifo);
    fclose(loader_fifo);
    remove(loader_fifo_path);

    /* Return					*/
    return status;
}

void signal_sender(int signal_number) {
    kill(instance_pid, signal_number);
}
