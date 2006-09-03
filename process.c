#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>

#include "libterm.h"

/* Return values:
 * EACCES, ENAMETOOLONG, ENOENT, ENOEXEC, ENOTDIR - see meaning
 *     of these errors at execve man page      
 */
int spawn(char * path) {
	pid_t pid;

	pid = fork();
	if(!pid) {
		execl(path, basename(path));

		if(errno == EACCES ||
                   errno == ENAMETOOLONG ||
                   errno == ENOENT ||
                   errno == ENOEXEC ||
                   errno == ENOTDIR) FIXABLE_SYS_ERR("execl", path)
		else FATAL_ERR("execl", path)
