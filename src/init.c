#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>

#include "libterm.h"
#include "process.h"
#include "window.h"

int next_tid = 0;
struct ltm_term_desc * descs = 0;
char init_state = INIT_NOT_DONE;

struct ltm_callbacks cbs;

#ifdef HAVE_LIBPTHREAD
pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t the_big_mutex;
pthread_t watchthread;
char threading = 0;
#endif

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
#ifdef HAVE_LIBPTHREAD
	pthread_mutexattr_t big_mutex_attrs;
#endif

	/* this should include some function to set off
	 * processing of the config file in the future!
	 */

	MUTEX_LOCK(init_mutex);
	if(init_state != INIT_NOT_DONE) return 0;
	init_state = INIT_IN_PROGRESS;
	MUTEX_UNLOCK(init_mutex);

	PTHREAD_CALL(pthread_mutexattr_init, (&big_mutex_attrs), NULL);
	PTHREAD_CALL(pthread_mutexattr_settype, (&big_mutex_attrs, PTHREAD_MUTEX_RECURSIVE), "PTHREAD_MUTEX_RECURSIVE");
	PTHREAD_CALL(pthread_mutex_init, (&the_big_mutex, &big_mutex_attrs), NULL);

	/* lock ASAP */
	LOCK_BIG_MUTEX;

	PTHREAD_CALL(pthread_mutexattr_destroy, (&big_mutex_attrs), NULL);

	if(setup_pipe() == -1) return -1;

	/* we're not really reloading it, but this does
	 * what we want to do, so use it
	 */
	if(reload_handler(SIGCHLD, dontfearthereaper) == -1) return -1;

	if(threading)
		if((errno = pthread_create(&watchthread, NULL, watch_for_events, NULL)))
			SYS_ERR("pthread_create", NULL);

	memset(&cbs, 0, sizeof(struct ltm_callbacks));

	init_state = INIT_DONE;

	UNLOCK_BIG_MUTEX;
	return 0;
}

int DLLEXPORT ltm_uninit() {
#ifdef HAVE_LIBPTHREAD
	void *ret;
	char code;
#endif

	MUTEX_LOCK(init_mutex);
	if(init_state != INIT_DONE) return 0;
	init_state = INIT_IN_PROGRESS;
	MUTEX_UNLOCK(init_mutex);

	LOCK_BIG_MUTEX;

#ifdef HAVE_LIBPTHREAD
	/* tell the thread to exit and then wait for it... */
	if(threading) {
		code = EXIT_THREAD;

		if(fwrite(&code, 1, sizeof(char), pipefiles[1]) < sizeof(char))
			SYS_ERR("fwrite", NULL);

		/* we need to temporarily unlock the mutex so that
		 * the thread will have a chance to process the event
		 */
		UNLOCK_BIG_MUTEX;
		PTHREAD_CALL(pthread_join, (watchthread, &ret), NULL);
		LOCK_BIG_MUTEX;

		if(!ret) {
			/* errors from the thread are put into a different
			 * struct error_info variable so that they aren't
			 * clobbered when another thread sets an error.
			 * Copy that error into ltm_curerr so that it'll
			 * be visible to the program.
			 */
			memcpy(&ltm_curerr, &thr_curerr, sizeof(struct error_info));
			return -1;
		}
	}
#endif

	/* close notification pipe */
	fclose(pipefiles[0]);
	fclose(pipefiles[1]);

	/* this should work... set the SIGCHLD handler
	 * to the sigaction struct used for simulation
	 */
	sigaction(SIGCHLD, &oldaction, NULL);

	UNLOCK_BIG_MUTEX;

	PTHREAD_CALL(pthread_mutex_destroy, (&the_big_mutex), NULL);

	/* init not done now */
	init_state = INIT_NOT_DONE;

	return 0;
}

/* errno values:
 * EPERM (Operation not permitted):
 * 	you neglected to call ltm_init() before calling this, you naughty person!
 */
int DLLEXPORT ltm_term_alloc() {
	int i, tid;

	CHECK_INITED;

	LOCK_BIG_MUTEX;

	for(i = 0; i < next_tid; i++)
		if(descs[i].allocated == 0) return i;

	/* no unused slots found, make a new one... */
	tid = next_tid;

	descs = realloc(descs, ++next_tid * sizeof(struct ltm_term_desc));
	if(!descs) SYS_ERR("realloc", NULL);

	memset(&descs[tid], 0, sizeof(struct ltm_term_desc));

	descs[tid].allocated = 1;

	UNLOCK_BIG_MUTEX;
	return tid;
}

/* errno values:
 * ENODEV: No available PTY devices were found.
 */
FILE DLLEXPORT * ltm_term_init(int tid) {
	struct passwd * pwd_entry;
	pid_t pid = -1;
	FILE *ret;
#ifdef HAVE_LIBPTHREAD
	char code;
#endif

	CHECK_INITED_PTR;

	LOCK_BIG_MUTEX_PTR;

	DIE_ON_INVAL_TID_PTR(tid)

	if(!cbs.update_areas) LTM_ERR_PTR(EPERM, "Terminal init cannot happen; callbacks haven't been set yet");

	if(!descs[tid].width || !descs[tid].height)
		if(ltm_set_window_dimensions(tid, 80, 24) == -1) return NULL;

	if(choose_pty_method(&descs[tid].pty) == -1) return NULL;

	if(tcsetwinsz(tid) == -1) return NULL;

	if(!descs[tid].shell_disabled) {
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
	}

#ifdef HAVE_LIBPTHREAD
	if(threading) {
		code = NEW_TERM;
		if(fwrite(&code, 1, sizeof(char), pipefiles[1]) < sizeof(char))
			SYS_ERR_PTR("fwrite", NULL);

		if(fwrite(&tid, 1, sizeof(int), pipefiles[1]) < sizeof(int))
			SYS_ERR_PTR("fwrite", NULL);
	}
#endif

	descs[tid].pid = pid;
	ret = descs[tid].pty.master;
	UNLOCK_BIG_MUTEX_PTR;
	return ret;
}

int DLLEXPORT ltm_close(int tid) {
	uint i;

	CHECK_INITED;

	LOCK_BIG_MUTEX;

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

	UNLOCK_BIG_MUTEX;
	return 0;
}

int DLLEXPORT ltm_set_shell(int tid, char * shell) {
	CHECK_INITED;

	LOCK_BIG_MUTEX;

	DIE_ON_INVAL_TID(tid)

	if(!shell) descs[tid].shell_disabled = 1;
	else {
		descs[tid].shell = strdup(shell);
		if(!descs[tid].shell) SYS_ERR("strdup", shell);
	}

	UNLOCK_BIG_MUTEX;
	return 0;
}

int DLLEXPORT ltm_set_threading(char val) {
	CHECK_NOT_INITED;

#ifdef HAVE_LIBPTHREAD
	threading = val;
#else
	if(val) LTM_ERR(ENOTSUP, "libterm was not compiled with threading support enabled");
#endif

	return 0;
}

FILE DLLEXPORT * ltm_get_notifier() {
	FILE *ret;

	CHECK_INITED_PTR;

	if(threading)
		LTM_ERR(ENOTSUP, "Threading is on, so you may not get the notification pipe");

	LOCK_BIG_MUTEX_PTR;

	ret = pipefiles[0];
	UNLOCK_BIG_MUTEX_PTR;
	return ret;
}
