#include <sys/types.h>
#include <unistd.h>

#include "libterm.h"

/* Start a program (the shell) using a different I/O source */
int spawn_unix(char * path, int io_fd) {
	pid_t pid;

	pid = fork();
	if(!pid) {
		dup2(io_fd, 0);
		dup2(io_fd, 1);
		dup2(io_fd, 2);
		
		if(io_fd > 2) close(io_fd);

		execl(path, path, NULL);

		FATAL_ERR("execl", path) /* if we get here, execl failed */
	}
	else if(pid == -1) FATAL_ERR("fork", NULL);

	return pid;
}

int spawn(char * path, FILE * io_file) {
	return spawn_unix(path, fileno(io_file));
}
