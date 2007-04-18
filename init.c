#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>

#include "libterm.h"

int next_desc = 0;
struct ltm_term_desc * descriptors = 0; /* no such struct yet... */

int ltm_init_with_shell(char * shell) {
	pid_t pid;

	/* get the slave FD here... */
	
	pid = spawn(shell);

	descriptors = realloc(descriptors, ++next_desc * sizeof(struct ltm_term_desc));

	/* probably fill in the struct here */

	return next_desc-1;
}

/* Return values:
 * EINVAL - current UID is not a valid user id - try using ltm_init_with_shell
 * to avoid having to access the passwd database
 */
int ltm_init() {
	struct passwd * pwd_entry;

	errno = 0; /* suggestion from getpwuid(3) */
	pwd_entry = getpwuid(getuid());
	/* I hear that many different things might be returned on a uid not
	 * found, depending on the UNIX system. This would cause problems.
	 * I've put in some of the possible errno values noted in the getpwuid
	 * manpage, but it's probably not all of them. If anyone finds that
	 * their system returns anything other than the values handled here,
	 * let me know.
	 */
	if(!pwd_entry &&
	   (errno == ENOENT || errno == ESRCH || errno == EBADF ||
	    errno == EPERM || errno == 0)) FATAL_ERR("getpwuid", getuid())

	return ltm_init_with_shell(pwd_entry->pw_shell);
}
