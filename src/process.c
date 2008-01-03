#include <sys/types.h>
#include <termios.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include "libterm.h"

#ifdef GWINSZ_IN_SYS_IOCTL
# include <sys/ioctl.h>
#endif

struct sigaction oldaction;
FILE * pipefiles[2];

/* Start a program (the shell) using a different I/O source */
static int spawn_unix(char * path, int io_fd) {
	sigset_t allsigs;
	pid_t pid;

	sigfillset(&allsigs);

	pid = fork();
	if(!pid) {
		/* We can't use normal error handling in here.
		 * Instead, just use exit(EXIT_FAILURE)
		 */

		sigprocmask(SIG_UNBLOCK, &allsigs, NULL);

		if(setsid() == -1) exit(EXIT_FAILURE);

		if(ioctl(io_fd, TIOCSCTTY, 0) == -1) exit(EXIT_FAILURE);

		if(dup2(io_fd, 0) == -1) exit(EXIT_FAILURE);
		if(dup2(io_fd, 1) == -1) exit(EXIT_FAILURE);
		if(dup2(io_fd, 2) == -1) exit(EXIT_FAILURE);

		if(io_fd > 2) if(close(io_fd) == -1) exit(EXIT_FAILURE);

		execl(path, path, NULL);

		exit(EXIT_FAILURE); /* if we get here, execl failed */
	}
	else if(pid == -1) SYS_ERR("fork", NULL);

	return pid;
}

int spawn(char * path, FILE * io_file) {
	return spawn_unix(path, fileno(io_file));
}

/* Here are some misc notes on accomplishing signal handler call simulation:
 *
 * Sorry if this isn't very comprehensible... I wrote it for my own benefit...
 *
 * Anytime we want to call a handler (sa_sigaction or sa_handler),
 * we need to first check whether it's SIG_DFL or SIG_IGN and not call
 * them in these cases. This is both because these will probably not
 * be valid function pointers and also because their action would conflict
 * with out usage. However, we don't want to simply ignore everything else
 * in these cases - for example, we still need to check whether SA_NOCLDWAIT
 * it set, since this means that whoever set that still wants children
 * to not produce zombies. (Note that now that we're simply setting
 * SA_NOCLDWAIT if it's set in their handler, this no longer applies).
 *
 * === For handling the SA_SIGINFO flag ===
 *
 * We're going to have our handler use SA_SIGINFO so
 * that we can support their handler with SA_SIGINFO both on and off.
 *
 * If sa_flags & SA_SIGINFO
 * 	call sa_sigaction with all the args to our handler
 * else
 * 	call sa_handler with only the first arg to our handler
 *
 * === For handling the SA_NOCLDSTOP flag ===
 *
 * This flag specifies that the signal handler will not be called if the
 * only thing that happened was that the child stopped or was continued.
 *
 * If sig == SIGCHLD and sa_flags & SA_NOCLDSTOP and
 * 		(si_code == CLD_STOPPED or si_code == CLD_CONTINUED)
 * 	don't call anything
 * else
 * 	call the handler as specified above
 *
 * This isn't entirely accurate as it doesn't prevent the WUNTRACED and
 * WCONTINUED flags to waitpid from working, but I assume that programs
 * specifying SA_NOCLDSTOP will just not try to use those two waitpid flags,
 * right? *looks around desperately...*
 *
 * A possibly more accurate way to simulate this would be to pass this
 * flag to our handler if their handler specified it and simply not do
 * anything if the handler was called because of a stopped/continued event.
 *
 * === For handling the SA_NOCLDWAIT flag ===
 *
 * This flag specifies that children don't turn into zombies if a wait
 * family function is not called. Therefore, we can simulate this by
 * conditionally calling waitpid ourselves:
 *
 * Before calling the handler, do:
 *
 * If sig == SIGCGLD and sa_flags & SA_NOCLDWAIT
 * 	waitpid(si_pid, NULL, 0);
 *
 * Do this before calling the handler since a wait function called in
 * the handler will expect to block.
 *
 * This isn't entirely accurate as it doesn't cause any of the wait
 * functions from blocking until all children have exited and then
 * returning -1 with ECHILD in errno. Unlike with SA_NOCLDSTOP,
 * however, we can't simply depend on the user not specifying some
 * flag to waitpid as there isn't one - the behavior should be
 * changed no matter what flags are specified. This could potentially
 * be a large issue since they might depend on the SA_NOCLDWAIT
 * behavior.
 *
 * It appears that calling a wait function in a SIGCHLD handler doesn't
 * prevent a wait function that was already blocking from receiving
 * a child changed state event, even though the first wait will have
 * caused the zombie to disappear. Thus, it isn't possible to emulate
 * the SA_NOCLDWAIT behavior with respect to the wait functions.
 *
 * A possibly more accurate way of simulating this would be to set
 * SA_NOCLDWAIT in our handler if it's set in their handler. Since
 * we don't depend on calling a wait function for status info, we
 * can simply depend on their handler to unzombify the children if
 * necessary. Setting the handler to SIG_IGN causes the same effect
 * as setting SA_NOCLDWAIT, so this can be simulated by setting
 * SA_NOCLDWAIT in our handler if their handler is set to SIG_IGN.
 */

static void simulate_handler_call(int sig, siginfo_t * info, void * data) {
	sigset_t oldmask;

	if(
		oldaction.sa_handler == SIG_DFL ||
		oldaction.sa_handler == SIG_IGN ||
		oldaction.sa_sigaction == (void*)SIG_DFL ||
		oldaction.sa_sigaction == (void*)SIG_IGN
	  ) return; /* these would conflict with our usage, ignore them */

	sigprocmask(SIG_SETMASK, &oldaction.sa_mask, &oldmask);
	if(oldaction.sa_flags & SA_SIGINFO)
		(*oldaction.sa_sigaction)(sig, info, data);
	else
		(*oldaction.sa_handler)(sig);
	sigprocmask(SIG_SETMASK, &oldmask, NULL);

	if(oldaction.sa_flags & SA_RESETHAND) {
		oldaction.sa_handler = SIG_DFL;
		sigemptyset(&oldaction.sa_mask);
		oldaction.sa_flags = 0;
	}
}

void dontfearthereaper(int sig, siginfo_t * info, void * data) {
#ifdef HAVE_LIBPTHREAD
	char code;
#endif
	int i;

	simulate_handler_call(sig, info, data);

	/* Does the process still exist? Yes.
	 * Is it not a zombie? Yes.
	 * Do I care? NO!!!!!
	 */
	if(
		info->si_code == CLD_STOPPED ||
		info->si_code == CLD_CONTINUED ||
		info->si_code == CLD_TRAPPED
	  ) return;

	for(i = 0; i < next_tid; i++)
		if(descs[i].pid == info->si_pid) {
			/* do some stuff that notifies various things that
			 * the shell has exited
			 */
#ifdef HAVE_LIBPTHREAD
			if(threading) {
				code = DEL_TERM;
				fwrite(&code, 1, sizeof(char), pipefiles[1]);
			}
#endif

			fwrite(&i, 1, sizeof(int), pipefiles[1]);
			break;
		}
}

/* don't call this before setting oldaction! */
int set_handler(int sig, void (*callback)(int, siginfo_t *, void *)) {
	struct sigaction action;

	action.sa_sigaction = callback;
	sigemptyset(&action.sa_mask);
	action.sa_flags = SA_SIGINFO;

	/* for this to work, we have to support the behavior with
	 * SA_RESTART both on and off. to do this, just make sure
	 * the syscall is restarted when returning -1 with EINTR
	 * in errno. if SA_RESTART is on, the syscall will be
	 * automatically restarted and this code will simply not
	 * get used.
	 */
	if(oldaction.sa_flags & SA_RESTART)
		action.sa_flags |= SA_RESTART;

	/* unlike most of the other flags, SA_NODEFER doesn't actually
	 * make something happen, rather it prevents something from
	 * happening. this "something" is the adding of the signal being
	 * handled to the signal mask.
	 *
	 * do this here so that the signal mask changing can be just a
	 * little more atomic
	 */
	if(!(oldaction.sa_flags & SA_NODEFER))
		if(sigaddset(&oldaction.sa_mask, sig) == -1) SYS_ERR("sigaddset", NULL);

	/* The handler being SIG_IGN causes the same effect as having
	 * SA_NOCLDWAIT set. We don't need to check whether the signal
	 * is SIGCHLD because SA_NOCLDWAIT only effects SIGCHLD anyway.
	 */
	if(
		oldaction.sa_flags & SA_NOCLDWAIT ||
		oldaction.sa_handler == SIG_IGN ||
		oldaction.sa_sigaction == (void*)SIG_IGN
	  ) action.sa_flags |= SA_NOCLDWAIT;

	if(oldaction.sa_flags & SA_NOCLDSTOP)
		action.sa_flags |= SA_NOCLDSTOP;

	/* and now... we pray that the new stack has enough
	 * space to hold two additional frames :-/
	 *
	 * two because one for simulate_handler_call and one
	 * for the call to the actual handler (which would
	 * normally be the first thing called)
	 *
	 * of course, the size of a frame depends a lot on
	 * how many local variables there are, and these
	 * functions don't have many...
	 */
	if(oldaction.sa_flags & SA_ONSTACK)
		action.sa_flags |= SA_ONSTACK;

	/* if sig isn't a valid signal, the above sigaddset call will
	 * have failed - no need to check for failure here
	 */
	sigaction(sig, &action, NULL);

	return 0;
}

int reload_handler(int sig, void (*callback)(int, siginfo_t *, void *)) {
	if(sigaction(sig, NULL, &oldaction) == -1) SYS_ERR("sigaction", NULL);

	return set_handler(sig, callback);
}

int set_handler_struct(int sig, void (*callback)(int, siginfo_t *, void *), struct sigaction * action) {
	memcpy(&oldaction, action, sizeof(struct sigaction));

	return set_handler(sig, callback);
}

/* This function is intended to be used in the case that an application needs
 * to call something (another library, for example) that it has no control over
 * and that changes the handler for SIGCHLD after the call to ltm_init. The use
 * of this function in this situation can produce a race condition if not
 * handled carefully. Here's how to manage it:
 *
 * CODE START
 *
 * sigset_t mask;
 *
 * sigemptyset(&mask);
 * sigaddset(&mask, SIGCHLD);
 * 
 * sigprocmask(SIG_BLOCK, &mask, NULL);
 *
 * j_random_function_that_changes_the_sigchld_handler(blaz, blitz, blargh);
 *
 * ltm_reload_sigchld_handler();
 *
 * sigprocmask(SIG_UNBLOCK, &mask, NULL);
 *
 * CODE END
 *
 * This is required because if a shell exits in the gap between the function
 * that changes the handler and the ltm_reload_sigchld_handler call, the SIGCHLD
 * signal will be handled by the handler that just got set rather than libterm's
 * handler. If the child that exited was a shell, this will cause libterm to be
 * completely unaware of this and it will think that this child is still running
 * and that the terminal it was associated with is still valid. Blocking SIGCHLD
 * before the function that sets a new handler and unblocking it after
 * ltm_reload_sigchld_handler solves this problem.
 */
int DLLEXPORT ltm_reload_sigchld_handler() {
	int ret;

	CHECK_INITED;

	LOCK_BIG_MUTEX;

	ret = reload_handler(SIGCHLD, dontfearthereaper);
	UNLOCK_BIG_MUTEX;
	return ret;
}

/* If the application has control over the code that sets the new handler, this is the
 * better function to use as it has no race condition problems.
 */
int DLLEXPORT ltm_set_sigchld_handler(struct sigaction * action) {
	int ret;

	CHECK_INITED;

	LOCK_BIG_MUTEX;

	ret = set_handler_struct(SIGCHLD, dontfearthereaper, action);
	UNLOCK_BIG_MUTEX;
	return ret;
}
