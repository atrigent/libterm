#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>

#include "libterm.h"

int spawn(char * path) {
	pid_t pid;

	pid = fork();
	if(!pid) {
		execl(path, basename(path));

		FATAL_ERR("execl", path)
