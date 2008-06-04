#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>

#ifndef GWINSZ_IN_SYS_IOCTL
# include <termios.h>
#endif

#include "libterm.h"
#include "screen.h"
#include "bitarr.h"

int DLLEXPORT ltm_is_line_wrapped(int tid, ushort line) {
	int ret;

	CHECK_INITED;

	LOCK_BIG_MUTEX;

	DIE_ON_INVAL_TID(tid);

	if(line > descs[tid].lines-1) LTM_ERR(EINVAL, "line is too large", after_lock);

	ret = bitarr_test_index(descs[tid].wrapped, line);

after_lock:
	UNLOCK_BIG_MUTEX;
before_anything:
	return ret;
}

/* a function like this should've been put in POSIX...
 * (minus the libterm-specific stuff, of course)
 */
int tcsetwinsz(int tid) {
	int mfd = fileno(descs[tid].pty.master), ret = 0;
	struct winsize size;

	size.ws_row = descs[tid].lines;
	size.ws_col = descs[tid].cols;
	size.ws_xpixel = size.ws_ypixel = 0; /* necessary? */

	if(ioctl(mfd, TIOCSWINSZ, &size) == -1) SYS_ERR("ioctl", "TIOCSWINSZ", error);
	
error:
	return ret;
}

int DLLEXPORT ltm_set_window_dimensions(int tid, ushort lines, ushort cols) {
	char big_changes;
	pid_t pgrp;
	int ret = 0;

	CHECK_INITED;

	LOCK_BIG_MUTEX;

	DIE_ON_INVAL_TID(tid);

	if(!cols || !lines) LTM_ERR(EINVAL, "cols or lines is zero", after_lock);

	/* if not changing anything, just pretend we did something and exit successfully immediately */
	if(cols == descs[tid].cols && lines == descs[tid].lines) goto after_lock;

	if(lines > descs[tid].lines && 0 /* lines_in_buffer(tid) */)
		big_changes = 1;
	else if(lines < descs[tid].lines && descs[tid].cursor.y >= lines)
		big_changes = 1;
	else
		big_changes = 0;

	if(set_screen_dimensions(tid, MAINSCREEN, lines, cols) == -1) PASS_ERR(after_lock);
	/* probably:
	 * set_screen_dimensions(tid, ALTSCREEN, lines, cols);
	 * in the future
	 * ... or more! (who knows?)
	 *
	 * even though everything else is enabled to support an alternate screen,
	 * I don't want to enable this as it's a fairly expensive operation, and
	 * what's the point of enabling it if it's not actually used yet?
	 */

	descs[tid].lines = lines;
	descs[tid].cols = cols;

	/* since the memory for a descriptor gets zero'd out and MAINSCREEN
	 * evaluates to zero, this should work even when cur_screen hasn't
	 * been explicitly set yet
	 *
	 * is it wrong to make an assumption like this? is it ok to perform
	 * unnecessary operations (explicitly setting cur_screen to MAINSCREEN)
	 * in the name of correctness? I don't know :(
	 */
	if(descs[tid].cur_screen == MAINSCREEN)
		descs[tid].screen = descs[tid].main_screen;
	else if(descs[tid].cur_screen == ALTSCREEN)
		descs[tid].screen = descs[tid].alt_screen;
	/* what to do if cur_screen contains an invalid value??? */

	/* only do this if the pty has been created and the shell has been started! */
	/* checking if pid is set is a quick'n'dirty way or checking whether ltm_term_init finished */
	if(descs[tid].pid) {
		if(tcsetwinsz(tid) == -1) PASS_ERR(after_lock);

		/* only do this stuff if there is actually a shell to signal */
		if(descs[tid].pid > 0) {
			pgrp = tcgetpgrp(fileno(descs[tid].pty.master));
			if(pgrp == -1) SYS_ERR("tcgetpgrp", NULL, after_lock);

			/* not caring whether this fails or not
			 * it can do so if there's currently no
			 * foreground process group or if we don't
			 * have permission to signal some processes
			 * in the process group
			 */
			killpg(pgrp, SIGWINCH);
		}

		/* don't do this before ltm_term_init since the callbacks
		 * aren't guaranteed to be set (and there isn't much point
		 * since there can't be anything on the screen before the
		 * shell starts)
		 */
		if(big_changes)
			cb_refresh_screen(tid);
	}

after_lock:
	UNLOCK_BIG_MUTEX;
before_anything:
	return ret;
}
