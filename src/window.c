#include <sys/ioctl.h>

#ifndef GWINSZ_IN_SYS_IOCTL
# include <termios.h>
#endif

#include "libterm.h"
#include "callbacks.h"
#include "screen.h"
#include "bitarr.h"

int DLLEXPORT ltm_is_line_wrapped(int tid, ushort line) {
	int ret;

	CHECK_INITED;

	LOCK_BIG_MUTEX;

	DIE_ON_INVAL_TID(tid);

	if(line > descs[tid].lines-1) LTM_ERR(EINVAL, "line is too large", after_lock);

	ret = bitarr_test_index(CUR_SCR(tid).wrapped, line);

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
	int ret = 0;

	CHECK_INITED;

	LOCK_BIG_MUTEX;

	DIE_ON_INVAL_TID(tid);

	if(!cols || !lines) LTM_ERR(EINVAL, "cols or lines is zero", after_lock);

	/* if not changing anything, just pretend we did something and exit successfully immediately */
	if(cols == descs[tid].cols && lines == descs[tid].lines) goto after_lock;

	if(lines > descs[tid].lines && 0 /* lines_in_buffer(tid) */)
		big_changes = 1;
	else if(lines < descs[tid].lines && CUR_SCR(tid).cursor.y >= lines)
		big_changes = 1;
	else
		big_changes = 0;

	descs[tid].lines = lines;
	descs[tid].cols = cols;

	if(descs[tid].next_sid) {
		if(screen_set_dimensions(tid, descs[tid].cur_screen, lines, cols) == -1)
			PASS_ERR(after_lock);
	} else {
		/* don't do anything else if the initial screens
		 * haven't been set up yet
		 */
		goto after_lock;
	}

	/* only do this if the pty has been created and the shell has been started! */
	/* checking if pid is set is a quick'n'dirty way or checking whether ltm_term_init finished */
	if(descs[tid].pid) {
		if(tcsetwinsz(tid) == -1) PASS_ERR(after_lock);

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
