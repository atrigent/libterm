#include <termios.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "libterm.h"
#include "bitarr.h"
#include "cursor.h"

#ifdef GWINSZ_IN_SYS_IOCTL
# include <sys/ioctl.h>
#endif

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

static int resize_cols(int tid, ushort lines, ushort cols, uint **screen) {
	ushort i, n;
	int ret = 0;

	if(descs[tid].cols != cols)
		for(i = 0; i < lines; i++) {
			screen[i] = realloc(screen[i], cols * sizeof(screen[0][0]));
			if(!screen[i]) SYS_ERR("realloc", NULL, error);

			if(cols > descs[tid].cols)
				for(n = descs[tid].cols; n < cols; n++)
					screen[i][n] = ' ';
		}

error:
	return ret;
}

static int set_screen_dimensions(int tid, char type, ushort lines, ushort cols) {
	uint **dscreen, ***screen;
	ushort i, diff, n;
	int ret = 0;

	switch(type) {
		case MAINSCREEN:
			screen = &descs[tid].main_screen;
			break;
		case ALTSCREEN:
			screen = &descs[tid].alt_screen;
			break;
	}

	dscreen = *screen;

	if(!descs[tid].cols || !descs[tid].lines) {
		*screen = dscreen = malloc(lines * sizeof(dscreen[0]));
		if(!dscreen) SYS_ERR("malloc", NULL, error);

		for(i = 0; i < lines; i++) {
			dscreen[i] = malloc(cols * sizeof(dscreen[0][0]));
			if(!dscreen[i]) SYS_ERR("malloc", NULL, error);

			for(n = 0; n < cols; n++)
				dscreen[i][n] = ' ';
		}

		if(bitarr_resize(&descs[tid].wrapped, 0, lines) == -1) PASS_ERR(error);
	} else if(lines < descs[tid].lines) {
		/* in order to minimize the work that's done,
		 * here are some orders in which to do things:
		 *
		 * if lines is being decreased:
		 * 	the lines at the top should be freed
		 * 	the lines should be moved up
		 * 	the lines array should be realloced
		 * 	the lines should be realloced (if cols is changing)
		 *
		 * if lines is being increased:
		 * 	the lines should be realloced (if cols is changing)
		 * 	the lines array should be realloced
		 * 	the lines should be moved down
		 * 	the lines at the top should be malloced
		 *
		 * if lines is not changing:
		 * 	the lines should be realloced (if cols is changing)
		 */

		if(descs[tid].cursor.y >= lines)
			diff = descs[tid].cursor.y - (lines-1);
		else
			diff = 0;

		for(i = 0; i < diff; i++)
			/* probably push lines into the buffer later */
			free(dscreen[i]);

		for(i = lines + diff; i < descs[tid].lines; i++)
			free(dscreen[i]);

/*		for(i = 0; i < lines; i++)
			dscreen[i] = dscreen[i+diff];*/

		if(diff) {
			memmove(dscreen, &dscreen[diff], lines * sizeof(dscreen[0]));

			if(type == MAINSCREEN) {
				cursor_rel_move(tid, UP, diff);

				bitarr_shift_left(descs[tid].wrapped, descs[tid].lines, diff);
			}
		}

		*screen = dscreen = realloc(dscreen, lines * sizeof(dscreen[0]));
		if(!dscreen) SYS_ERR("realloc", NULL, error);

		if(bitarr_resize(&descs[tid].wrapped, descs[tid].lines, lines) == -1) PASS_ERR(error);

		if(resize_cols(tid, lines, cols, dscreen) == -1) PASS_ERR(error);
	} else if(lines > descs[tid].lines) {
		/* in the future this will look something like this:
		 *
		 * if(lines_in_buffer(tid)) {
		 * 	diff = lines - descs[tid].lines;
		 *
		 * 	if(lines_in_buffer(tid) < diff) diff = lines_in_buffer(tid);
		 * } else
		 * 	diff = 0;
		 *
		 * but for now, we're just putting the diff = 0 thing there
		 */

		diff = 0;

		if(resize_cols(tid, descs[tid].lines, cols, dscreen) == -1) PASS_ERR(error);

		*screen = dscreen = realloc(dscreen, lines * sizeof(dscreen[0]));
		if(!dscreen) SYS_ERR("realloc", NULL, error);

		if(bitarr_resize(&descs[tid].wrapped, descs[tid].lines, lines) == -1) PASS_ERR(error);

/*		for(i = diff; i < lines; i++)
			dscreen[i] = dscreen[i-diff];*/

		if(diff) {
			memmove(&dscreen[diff], dscreen, descs[tid].lines * sizeof(dscreen[0]));

			if(type == MAINSCREEN) {
				cursor_rel_move(tid, DOWN, diff);

				bitarr_shift_right(descs[tid].wrapped, descs[tid].lines, diff);
			}
		}

		/* probably pop lines off the buffer and put them in here later */
		for(i = 0; i < diff; i++) {
			dscreen[i] = malloc(cols * sizeof(dscreen[0][0]));
			if(!dscreen[i]) SYS_ERR("malloc", NULL, error);

			for(n = 0; n < cols; n++)
				dscreen[i][n] = ' ';
		}

		for(i = descs[tid].lines + diff; i < lines; i++) {
			dscreen[i] = malloc(cols * sizeof(dscreen[0][0]));
			if(!dscreen[i]) SYS_ERR("malloc", NULL, error);

			for(n = 0; n < cols; n++)
				dscreen[i][n] = ' ';
		}
	} else
		if(resize_cols(tid, lines, cols, dscreen) == -1) PASS_ERR(error);

error:
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

int DLLEXPORT ltm_set_window_dimensions(int tid, ushort cols, ushort lines) {
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

int scroll_screen(int tid) {
	uint i, n, *linesave;

	/* push this line into the buffer later... */
	linesave = descs[tid].screen[0];
	for(i = 0; i < descs[tid].cols; i++)
		linesave[i] = ' ';

	memmove(descs[tid].screen, &descs[tid].screen[1], sizeof(linesave) * (descs[tid].lines-1));

	descs[tid].screen[descs[tid].lines-1] = linesave;

	bitarr_shift_left(descs[tid].wrapped, descs[tid].lines, 1);

	for(i = 0; i < descs[tid].nareas;)
		if(!descs[tid].areas[i]->end.y) {
			/* this update has been scrolled off,
			 * free it and rotate the areas down
			 */
			free(descs[tid].areas[i]);

			descs[tid].nareas--;

			for(n = i; n < descs[tid].nareas; n++) descs[tid].areas[n] = descs[tid].areas[n+1];
		} else {
			/* move it down... */
			if(descs[tid].areas[i]->start.y)
				/* if it's not at the top yet, move it one up */
				descs[tid].areas[i]->start.y--;
			else
				/* if it's already at the top, don't move it;
				 * set the X coord to the beginning. Think
				 * about it! :)
				 */
				descs[tid].areas[i]->start.x = 0;

			descs[tid].areas[i]->end.y--;

			i++;
		}

	descs[tid].lines_scrolled++;

	return 0;
}
