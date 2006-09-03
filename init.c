#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <pwd.h>

#include "libterm.h"

/* Return values:
 * EINVAL - current UID is not a valid user id
 */
int ltm_init() {
	struct passwd * pwd_entry;

	pwd_entry = getpwuid(getuid());
	/* I hear that many different things might be returned on a uid not
	 * found, depending on the UNIX system. This would cause problems.
	 * I've put in some of the possible errno values noted in the getpwuid
	 * manpage, but it's probably not all of them. If anyone finds that
	 * their system returns anything other than the values handled here,
	 * let me know.
	 */
	if(!pwd_entry ||
	   (pwd_entry == -1 && 
	    (errno == ENOENT || errno == ESRCH || errno == EBADF ||
	     errno == EPERM)
	   )) FIXABLE_LTM_ERR(EINVAL)
	else if(pwd_entry == -1) FATAL_ERR("getpwuid", getuid())
