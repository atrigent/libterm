#include <string.h>
#include <stdlib.h>

#include "libterm.h"
#include "process.h"
#include "bitarr.h"

int DLLEXPORT ltm_feed_input_to_program(int tid, char * input, uint n) {
	DIE_ON_INVAL_TID(tid)

	if(fwrite(input, 1, n, descs[tid].pty.master) < n)
		SYS_ERR("fwrite", input);

	return 0;
}

int DLLEXPORT ltm_set_shell(int tid, char * shell) {
	DIE_ON_INVAL_TID(tid)

	descs[tid].shell = strdup(shell);

	if(!descs[tid].shell) SYS_ERR("strdup", shell);

	return 0;
}

struct ltm_callbacks DLLEXPORT * ltm_get_callbacks_ptr(int tid) {
	DIE_ON_INVAL_TID_PTR(tid)

	return &descs[tid].cb;
}

int DLLEXPORT ltm_toggle_threading(int tid) {
	DIE_ON_INVAL_TID(tid)

	descs[tid].threading = !descs[tid].threading;

	return 0;
}

int DLLEXPORT ltm_is_line_wrapped(int tid, ushort line) {
	DIE_ON_INVAL_TID(tid)

	if(line > descs[tid].height-1) LTM_ERR(EINVAL, "line is too large");

	return bitarr_test_index(descs[tid].wrapped, line);
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
	if(!init_done) LTM_ERR(EPERM, "libterm has not yet been inited");

	return reload_handler(SIGCHLD, dontfearthereaper);
}

/* If the application has control over the code that sets the new handler, this is the
 * better function to use as it has no race condition problems.
 */
int DLLEXPORT ltm_set_sigchld_handler(struct sigaction * action) {
	if(!init_done) LTM_ERR(EPERM, "libterm has not yet been inited");

	return set_handler_struct(SIGCHLD, dontfearthereaper, action);
}

FILE DLLEXPORT * ltm_get_notifier() {
	if(!init_done) LTM_ERR_PTR(EPERM, "libterm has not yet been inited");

	return pipefiles[0];
}

void DLLEXPORT ltm_set_error_dest(FILE * dest) {
	dump_dest = dest;
}
