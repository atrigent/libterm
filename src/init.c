#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>

#include "libterm.h"
#include "process.h"
#include "window.h"
#include "callbacks.h"
#include "threading.h"

int next_tid = 0;
struct ltm_term_desc * descs = 0;
char init_done = 0;

struct ltm_callbacks cbs;

pthread_t watchthread;
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

	if(threading)
		if((errno = pthread_create(&watchthread, NULL, watch_for_events, NULL)))
			SYS_ERR("pthread_create", NULL);

	memset(&cbs, 0, sizeof(struct ltm_callbacks));

	init_done = 1;

	return 0;
}

int DLLEXPORT ltm_uninit() {
	void *ret;
	char code;

	if(!init_done) return 0;

	/* tell the thread to exit and then wait for it... */
	if(threading) {
		code = EXIT_THREAD;

		if(fwrite(&code, 1, sizeof(char), pipefiles[1]) < sizeof(char))
			SYS_ERR("fwrite", NULL);

		/* pthread_join man page basically says that something
		 * other than zero will be returned on error
		 *
		 * also, the error is not set in errno but rather
		 * return. manually put it in errno.
		 */
		if((errno = pthread_join(watchthread, &ret)))
			SYS_ERR("pthread_join", NULL);

		/* FIXME!!!!!!!!!!
		 *
		 * this is WRONG. since the watch thread sets ltm_curerr just like
		 * any other thread, there is no way of knowing whether some other
		 * thread has been set an error condition between when the watch thread exited
		 * and now. therefore, if the program checks ltm_curerr after ltm_uninit
		 * returns, ltm_curerr might contain info for the wrong error!!!
		 */
		if(!ret) return -1;
	}

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
	char code;
	pid_t pid;

	DIE_ON_INVAL_TID_PTR(tid)

	if(!cbs.update_areas) LTM_ERR_PTR(EPERM, "Terminal init cannot happen; callbacks haven't been set yet");

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
		code = NEW_TERM;
		if(fwrite(&code, 1, sizeof(char), pipefiles[1]) < sizeof(char))
			SYS_ERR_PTR("fwrite", NULL);

		if(fwrite(&tid, 1, sizeof(int), pipefiles[1]) < sizeof(int))
			SYS_ERR_PTR("fwrite", NULL);
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
