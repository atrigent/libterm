#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "libterm.h"

/* Start a program (the shell) using a different I/O source */
int spawn_unix(char * path, int io_fd) {
	pid_t pid;

	pid = fork();
	if(!pid) {
		/* We can't use normal error handling in here.
		 * Instead, just use exit(EXIT_FAILURE)
		 */

		if(dup2(io_fd, 0) == -1) exit(EXIT_FAILURE);
		if(dup2(io_fd, 1) == -1) exit(EXIT_FAILURE);
		if(dup2(io_fd, 2) == -1) exit(EXIT_FAILURE);
		
		if(io_fd > 2) if(close(io_fd) == -1) exit(EXIT_FAILURE);

		execl(path, path, NULL);

		exit(EXIT_FAILURE); /* if we get here, execl failed */
	}
	else if(pid == -1) FATAL_ERR("fork", NULL);

	return pid;
}

int spawn(char * path, FILE * io_file) {
	return spawn_unix(path, fileno(io_file));
}
