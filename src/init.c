#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>

#include "libterm.h"
#include "process.h"
#include "window.h"
#include "callbacks.h"

int next_tid = 0;
struct ltm_term_desc * descs = 0;
char init_done = 0;

char threading = 0;

static int setup_pipe() {
	int pipefds[2];

	if(pipe(pipefds) == -1) SYS_ERR("pipe", NULL);

	pipefiles[0] = fdopen(pipefds[0], "r");
	pipefiles[1] = fdopen(pipefds[1], "w");

	if(!pipefiles[0] || !pipefiles[1]) SYS_ERR("fdopen", NULL);

	/* necessary...? */
	setbuf(pipefiles[0], NULL);
	setbuf(pipefiles[1], NULL);

	return 0;
}

int DLLEXPORT ltm_init() {
	/* this should include some function to set off
	 * processing of the config file in the future!
	 */

	if(init_done) return 0;

	if(!dump_dest) dump_dest = stderr;

	if(setup_pipe() == -1) return -1;

	/* we're not really reloading it, but this does
	 * what we want to do, so use it
	 */
	if(reload_handler(SIGCHLD, dontfearthereaper) == -1) return -1;

	init_done = 1;

	return 0;
}

int DLLEXPORT ltm_uninit() {
	if(!init_done) return 0;

	/* close notification pipe */
	fclose(pipefiles[0]);
	fclose(pipefiles[1]);

	/* this should work... set the SIGCHLD handler
	 * to the sigaction struct used for simulation
	 */
	sigaction(SIGCHLD, &oldaction, NULL);

	/* init not done now */
	init_done = 0;

	return 0;
}

/* errno values:
 * EPERM (Operation not permitted):
 * 	you neglected to call ltm_init() before calling this, you naughty person!
 */
int DLLEXPORT ltm_term_alloc() {
	int i, tid;

	if(!init_done) LTM_ERR(EPERM, "libterm has not yet been inited");

	for(i = 0; i < next_tid; i++)
		if(descs[i].allocated == 0) return i;

	/* no unused slots found, make a new one... */
	tid = next_tid;

	descs = realloc(descs, ++next_tid * sizeof(struct ltm_term_desc));
	if(!descs) SYS_ERR("realloc", NULL);

	memset(&descs[tid], 0, sizeof(struct ltm_term_desc));

	descs[tid].allocated = 1;

	return tid;
}

/* errno values:
 * ENODEV: No available PTY devices were found.
 */
FILE DLLEXPORT * ltm_term_init(int tid) {
	struct passwd * pwd_entry;
	pid_t pid;

	DIE_ON_INVAL_TID_PTR(tid)

	if(check_callbacks(tid) == -1) return NULL;

	if(!descs[tid].width || !descs[tid].height)
		if(ltm_set_window_dimensions(tid, 80, 24) == -1) return NULL;

	if(choose_pty_method(&descs[tid].pty) == -1) return NULL;

	if(tcsetwinsz(tid) == -1) return NULL;

	if(descs[tid].shell) {
		pid = spawn(descs[tid].shell, descs[tid].pty.slave);

		free(descs[tid].shell);
		descs[tid].shell = 0;
	} /*else if(config.shell)
		pid = spawn(config.shell, descs[tid].pty.slave);*/
	else {
		pwd_entry = getpwuid(getuid());
		if(!pwd_entry) SYS_ERR_PTR("getpwuid", NULL);

		pid = spawn(pwd_entry->pw_shell, descs[tid].pty.slave);
	}

	if(pid == -1)
		return NULL;

	if(threading) {
		/* do a bunch of shit to start up the thread */
	}

	descs[tid].pid = pid;
	return descs[tid].pty.master;
}

int DLLEXPORT ltm_close(int tid) {
	uint i;

	DIE_ON_INVAL_TID(tid)

	/* is this the right way to close these...? */
	fclose(descs[tid].pty.master);
	fclose(descs[tid].pty.slave);

	for(i=0; i < descs[tid].height; i++)
		free(descs[tid].main_screen[i]);

	free(descs[tid].main_screen);

	free(descs[tid].wrapped);

	if(tid == next_tid-1) {
		descs = realloc(descs, --next_tid * sizeof(struct ltm_term_desc));
		if(!descs && next_tid) SYS_ERR("realloc", NULL);
	} else
		memset(&descs[tid], 0, sizeof(struct ltm_term_desc));

	return 0;
}
