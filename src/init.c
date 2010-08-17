#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>

#include "libterm.h"
#include "linkedlist.h"
#include "callbacks.h"
#include "process.h"
#include "screen.h"
#include "window.h"
#include "module.h"
#include "idarr.h"
#include "conf.h"

int next_tid = 0;
struct term_desc *descs = 0;
enum initstate init_state = INIT_NOT_DONE;

struct callbacks cbs;

#ifdef HAVE_LIBPTHREAD
pthread_mutex_t test_and_set_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t the_big_mutex;
pthread_t watchthread;
char threading = 0;
#endif

static int setup_pipe() {
	int pipefds[2], ret = 0;

	if(pipe(pipefds) == -1) SYS_ERR("pipe", NULL, error);

	pipefiles[0] = fdopen(pipefds[0], "r");
	pipefiles[1] = fdopen(pipefds[1], "w");

	if(!pipefiles[0] || !pipefiles[1]) SYS_ERR("fdopen", NULL, error);

	/* necessary...? */
	setbuf(pipefiles[0], NULL);
	setbuf(pipefiles[1], NULL);

error:
	return ret;
}

int DLLEXPORT ltm_init() {
#ifdef HAVE_LIBPTHREAD
	pthread_mutexattr_t big_mutex_attrs;
#endif
	int ret = 0;

	MUTEX_LOCK(test_and_set_mutex, before_anything);
	if(init_state != INIT_NOT_DONE) {
		MUTEX_UNLOCK(test_and_set_mutex, before_anything);
		return 0;
	}
	init_state = INIT_IN_PROGRESS;
	MUTEX_UNLOCK(test_and_set_mutex, before_anything);

	PTHREAD_CALL(pthread_mutexattr_init, (&big_mutex_attrs), NULL, before_anything);
	PTHREAD_CALL(pthread_mutexattr_settype, (&big_mutex_attrs, PTHREAD_MUTEX_RECURSIVE), "PTHREAD_MUTEX_RECURSIVE", before_anything);
	PTHREAD_CALL(pthread_mutex_init, (&the_big_mutex, &big_mutex_attrs), NULL, before_anything);

	/* lock ASAP */
	LOCK_BIG_MUTEX;

	PTHREAD_CALL(pthread_mutexattr_destroy, (&big_mutex_attrs), NULL, after_lock);

	LTDL_CALL(init, (), after_lock);
	LTDL_CALL(addsearchdir, (MODULE_PATH), after_lock);

	if(config_init() == -1) PASS_ERR(after_lock);

	if(setup_pipe() == -1) PASS_ERR(after_lock);

	/* we're not really reloading it, but this does
	 * what we want to do, so use it
	 */
	if(reload_handler(SIGCHLD, dontfearthereaper) == -1) PASS_ERR(after_lock);

#ifdef HAVE_PTHREAD
	if(threading)
		PTHREAD_CALL(pthread_create, (&watchthread, NULL, watch_for_events, NULL), NULL, after_lock);
#endif

	memset(&cbs, 0, sizeof(struct callbacks));

	init_state = INIT_DONE;

after_lock:
	UNLOCK_BIG_MUTEX;
before_anything:
	return ret;
}

int DLLEXPORT ltm_uninit() {
#ifdef HAVE_LIBPTHREAD
	enum threadaction code;
	void *thr_ret;
#endif
	int ret = 0;

	MUTEX_LOCK(test_and_set_mutex, before_anything);
	if(init_state != INIT_DONE) {
		MUTEX_UNLOCK(test_and_set_mutex, before_anything);
		return 0;
	}
	init_state = INIT_IN_PROGRESS;
	MUTEX_UNLOCK(test_and_set_mutex, before_anything);

	LOCK_BIG_MUTEX;

#ifdef HAVE_LIBPTHREAD
	/* tell the thread to exit and then wait for it... */
	if(threading) {
		code = EXIT_THREAD;

		if(fwrite(&code, 1, sizeof(enum threadaction), pipefiles[1]) < sizeof(enum threadaction))
			SYS_ERR("fwrite", NULL, after_lock);

		/* we need to temporarily unlock the mutex so that
		 * the thread will have a chance to process the event
		 */
		UNLOCK_BIG_MUTEX;
		PTHREAD_CALL(pthread_join, (watchthread, &thr_ret), NULL, before_anything);
		LOCK_BIG_MUTEX;

		if(!thr_ret) {
			/* errors from the thread are put into a different
			 * struct error_info variable so that they aren't
			 * clobbered when another thread sets an error.
			 * Copy that error into ltm_curerr so that it'll
			 * be visible to the program.
			 */
			memcpy(&ltm_curerr, &thr_curerr, sizeof(struct error_info));
			PASS_ERR(after_lock);
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

	config_free();

	flush_repeated_errors();

	LTDL_CALL(exit, (), after_lock);

after_lock:
	UNLOCK_BIG_MUTEX;

	/* if we're here because of error, we don't want to set
	 * INIT_STATE or destroy the mutex */
	if(ret == -1) goto before_anything;

	PTHREAD_CALL(pthread_mutex_destroy, (&the_big_mutex), NULL, before_anything);

	/* init not done now */
	init_state = INIT_NOT_DONE;

before_anything:
	return ret;
}

/* errno values:
 * EPERM (Operation not permitted):
 * 	you neglected to call ltm_init() before calling this, you naughty person!
 */
int DLLEXPORT ltm_term_alloc() {
	int ret;

	CHECK_INITED;

	LOCK_BIG_MUTEX;

	ret = IDARR_ID_ALLOC(descs, next_tid);

	UNLOCK_BIG_MUTEX;
before_anything:
	return ret;
}

/* errno values:
 * ENODEV: No available PTY devices were found.
 */
FILE DLLEXPORT *ltm_term_init(int tid) {
	struct passwd *pwd_entry;
	pid_t pid = -1;
	FILE *ret;
#ifdef HAVE_LIBPTHREAD
	enum threadaction code;
#endif

	CHECK_INITED_PTR;

	LOCK_BIG_MUTEX_PTR;

	DIE_ON_INVAL_TID_PTR(tid);

	if(!cbs.update_ranges) LTM_ERR_PTR(EPERM, "Terminal init cannot happen; callbacks haven't been set yet", after_lock);

	if(choose_pty_method(&descs[tid].pty) == -1) PASS_ERR_PTR(after_lock);

	if(!descs[tid].cols || !descs[tid].lines) {
		/* don't hardcode these in the future */
		descs[tid].lines = 24;
		descs[tid].cols = 80;
	}

	descs[tid].cur_screen = descs[tid].cur_input_screen =
		screen_alloc(tid, 1, descs[tid].lines, descs[tid].cols);
	if(descs[tid].cur_screen == -1) PASS_ERR_PTR(after_lock);

	if(tcsetwinsz(tid) == -1) PASS_ERR_PTR(after_lock);

	if(!descs[tid].shell_disabled) {
		if(descs[tid].shell) {
			pid = spawn(descs[tid].shell, descs[tid].pty.slave);

			free(descs[tid].shell);
			descs[tid].shell = 0;
		} /*else if(config.shell)
			pid = spawn(config.shell, descs[tid].pty.slave);*/
		else {
			pwd_entry = getpwuid(getuid());
			if(!pwd_entry) SYS_ERR_PTR("getpwuid", NULL, after_lock);

			pid = spawn(pwd_entry->pw_shell, descs[tid].pty.slave);
		}

		if(pid == -1) PASS_ERR_PTR(after_lock);
	}

#ifdef HAVE_LIBPTHREAD
	if(threading) {
		code = NEW_TERM;
		if(fwrite(&code, 1, sizeof(enum threadaction), pipefiles[1]) < sizeof(enum threadaction))
			SYS_ERR_PTR("fwrite", NULL, after_lock);

		if(fwrite(&tid, 1, sizeof(int), pipefiles[1]) < sizeof(int))
			SYS_ERR_PTR("fwrite", NULL, after_lock);
	}
#endif

	descs[tid].pid = pid;
	ret = descs[tid].pty.master;
after_lock:
	UNLOCK_BIG_MUTEX_PTR;
before_anything:
	return ret;
}

int DLLEXPORT ltm_close(int tid) {
	int ret = 0;
	uint i;
	int n;

	CHECK_INITED;

	LOCK_BIG_MUTEX;

	DIE_ON_INVAL_TID(tid);

	/* is this the right way to close these...? */
	fclose(descs[tid].pty.master);
	fclose(descs[tid].pty.slave);

	for(n = 0; n < descs[tid].next_sid; n++)
		if(descs[tid].screens[n].allocated) {
			for(i=0; i < descs[tid].screens[n].lines; i++)
				free(descs[tid].screens[n].matrix[i]);

			linkedlist_free(&descs[tid].screens[n].uplinks);
			linkedlist_free(&descs[tid].screens[n].downlinks);

			free(descs[tid].screens[n].matrix);
			free(descs[tid].screens[n].wrapped);
		}

	free(descs[tid].screens);

	if(IDARR_ID_DEALLOC(descs, next_tid, tid) == -1)
		PASS_ERR(after_lock);

after_lock:
	UNLOCK_BIG_MUTEX;
before_anything:
	return ret;
}

int DLLEXPORT ltm_set_shell(int tid, char *shell) {
	int ret = 0;

	CHECK_INITED;

	LOCK_BIG_MUTEX;

	DIE_ON_INVAL_TID(tid);

	if(!shell) descs[tid].shell_disabled = 1;
	else {
		descs[tid].shell = strdup(shell);
		if(!descs[tid].shell) SYS_ERR("strdup", shell, after_lock);
	}

after_lock:
	UNLOCK_BIG_MUTEX;
before_anything:
	return ret;
}

int DLLEXPORT ltm_set_threading(char val) {
	int ret = 0;

	CHECK_NOT_INITED;

#ifdef HAVE_LIBPTHREAD
	threading = val;
#else
	if(val) LTM_ERR(ENOTSUP, "libterm was not compiled with threading support enabled", before_anything);
#endif

before_anything:
	return ret;
}

FILE DLLEXPORT *ltm_get_notifier() {
	FILE *ret;

	CHECK_INITED_PTR;

#ifdef HAVE_LIBPTHREAD
	if(threading)
		LTM_ERR_PTR(ENOTSUP, "Threading is on, so you may not get the notification pipe", before_anything);
#endif

	LOCK_BIG_MUTEX_PTR;

	ret = pipefiles[0];
	UNLOCK_BIG_MUTEX_PTR;
before_anything:
	return ret;
}
