#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>

#include "libterm.h"
#include "process.h"
#include "interface.h"

int next_tid = 0;
struct ltm_term_desc * descriptors = 0;
char init_done = 0;

int ltm_init() {
	/* this should include some function to set off
	 * processing of the config file in the future!
	 */

	dump_dest = stderr;

	/* we're not really reloading it, but this does
	 * what we want to do, so use it
	 */
	if(ltm_reload_sigchld_handler() == -1) return -1;

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

/* errno values:
 * ENODEV: No available PTY devices were found.
 */
int ltm_term_init(int tid) {
	struct passwd * pwd_entry;
	pid_t pid;

	DIE_ON_INVAL_TID(tid)

	if(!descriptors[tid].width || !descriptors[tid].height)
		if(ltm_set_window_dimensions(tid, 80, 24) == -1) return -1;

	if(choose_pty_method(&descriptors[tid].pty) == -1) return -1;

	if(descriptors[tid].shell) {
		pid = spawn(descriptors[tid].shell, descriptors[tid].pty.slave);

		free(descriptors[tid].shell);
		descriptors[tid].shell = 0;
	} /*else if(config.shell)
		pid = spawn(config.shell, descriptors[tid].pty.slave);*/
	else {
		pwd_entry = getpwuid(getuid());
		if(!pwd_entry) FATAL_ERR("getpwuid", NULL)

		pid = spawn(pwd_entry->pw_shell, descriptors[tid].pty.slave);
	}

	if(pid == -1)
		return -1;
	else {
		descriptors[tid].pid = pid;
		return 0;
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
