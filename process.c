#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>

#include "libterm.h"

/* Start a program (the shell) using a different I/O source */
int spawn(char * path, int io_fd) {
	pid_t pid;

	pid = fork();
	if(!pid) {
		dup2(io_fd, 0);
		dup2(io_fd, 1);
		dup2(io_fd, 2);
		
		if(io_fd > 2) close(io_fd);

		execl(path, basename(path));

		FATAL_ERR("execl", path) /* if we get here, execl failed */
	}
	else if(pid == -1) FATAL_ERR("fork", NULL);

	return pid;
}
