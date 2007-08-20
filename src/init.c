#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>

#include "libterm.h"

int next_tid = 0;
struct ltm_term_desc * descriptors = 0;
char init_done = 0;

int ltm_init() {
	/* this should include some function to set off
	 * processing of the config file in the future!
	 */

	dump_dest = stderr;

	init_done = 1;

	return 0;
}

/* errno values:
 * EPERM (Operation not permitted):
 * 	you neglected to call ltm_init() before calling this, you naughty person!
 */
int ltm_term_alloc() {
	int i, tid;

	if(!init_done) FIXABLE_LTM_ERR(EPERM)

	for(i = 0; i < next_tid; i++)
		if(descriptors[i].allocated == 0) return i;

	/* no unused slots found, make a new one... */
	tid = next_tid;

	descriptors = realloc(descriptors, ++next_tid * sizeof(struct ltm_term_desc));
	memset(&descriptors[tid], 0, sizeof(struct ltm_term_desc));

	descriptors[tid].allocated = 1;

	return tid;
}

int term_init_with_shell(int tid, char * shell) {
	pid_t pid;
	uint i, n;

	DIE_ON_INVAL_TID(tid)

	if(choose_pty_method(&descriptors[tid].pty) == -1) return -1;
	
	pid = spawn(shell, descriptors[tid].pty.slave);

	return tid;
}

/* errno values:
 * ENODEV: No available PTY devices were found.
 */
int ltm_term_init(int tid) {
	struct passwd * pwd_entry;
	int ret;

	DIE_ON_INVAL_TID(tid)

	if(descriptors[tid].shell) {
		ret = term_init_with_shell(tid, descriptors[tid].shell);

		free(descriptors[tid].shell);
		descriptors[tid].shell = 0;

		return ret;
	}
	/* else if(config.shell) {
		ret = term_init_with_shell(tid, config.shell);

		free(config.shell);
		config.shell = 0;

		return ret;
	}*/
	else {
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
		    errno == EPERM || errno == 0)) FATAL_ERR("getpwuid", "your current UID")

		return term_init_with_shell(tid, pwd_entry->pw_shell);
	}
}

int ltm_close(int tid) {
	uint i;

	DIE_ON_INVAL_TID(tid)

	/* is this the right way to close these...? */
	fclose(descriptors[tid].pty.master);
	fclose(descriptors[tid].pty.slave);

	for(i=0; i < descriptors[tid].height; i++)
		free(descriptors[tid].main_screen[i]);

	free(descriptors[tid].main_screen);

	if(tid == next_tid-1)
		descriptors = realloc(descriptors, --next_tid * sizeof(struct ltm_term_desc));
	else
		memset(&descriptors[tid], 0, sizeof(struct ltm_term_desc));

	return 0;
}
