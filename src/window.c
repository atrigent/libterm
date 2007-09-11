#include <termios.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "libterm.h"
#include "callbacks.h"

#ifdef GWINSZ_IN_SYS_IOCTL
# include <sys/ioctl.h>
#endif

int resize_width(int tid, ushort width, ushort height, uint ** screen) {
	ushort i, n;

	if(descriptors[tid].width != width)
		for(i = 0; i < height; i++) {
			screen[i] = realloc(screen[i], width * sizeof(uint));
			if(!screen[i]) FATAL_ERR("realloc", NULL)

			if(width > descriptors[tid].width)
				for(n = descriptors[tid].width; n < width; n++)
					screen[i][n] = ' ';
		}

	return 0;
}

int set_screen_dimensions(int tid, char type, ushort width, ushort height) {
	uint **dscreen, ***screen;
	ushort i, diff, n;

	switch(type) {
		case MAINSCREEN:
			screen = &descriptors[tid].main_screen;
			break;
		case ALTSCREEN:
			screen = &descriptors[tid].alt_screen;
			break;
	}

	dscreen = *screen;

	if(!descriptors[tid].width || !descriptors[tid].height) {
		*screen = dscreen = malloc(height * sizeof(uint *));
		if(!dscreen) FATAL_ERR("malloc", NULL)

		for(i = 0; i < height; i++) {
			dscreen[i] = malloc(width * sizeof(uint));
			if(!dscreen[i]) FATAL_ERR("malloc", NULL)

			for(n = 0; n < width; n++)
				dscreen[i][n] = ' ';
		}
	} else if(height < descriptors[tid].height) {
		/* in order to minimize the work that's done,
		 * here are some orders in which to do things:
		 *
		 * if the height is being decreased:
		 * 	the lines at the top should be freed
		 * 	the lines should be moved up
		 * 	the lines array should be realloced
		 * 	the lines should be realloced (if the width is changing)
		 *
		 * if the height is being increased:
		 * 	the lines should be realloced (if the width is changing)
		 * 	the lines array should be realloced
		 * 	the lines should be moved down
		 * 	the lines at the top should be malloced
		 *
		 * if the height is not changing:
		 * 	the lines should be realloced (if the width is changing)
		 */

		if(descriptors[tid].cursor.y >= height)
			diff = descriptors[tid].cursor.y - (height-1);
		else
			diff = 0;

		for(i = 0; i < diff; i++)
			/* probably push lines into the buffer later */
			free(dscreen[i]);

		for(i = height + diff; i < descriptors[tid].height; i++)
			free(dscreen[i]);

/*		for(i = 0; i < height; i++)
			dscreen[i] = dscreen[i+diff];*/

		if(diff) {
			memmove(dscreen, &dscreen[diff], height * sizeof(uint *));

			if(type == MAINSCREEN)
				descriptors[tid].cursor.y -= diff;
		}

		*screen = dscreen = realloc(dscreen, height * sizeof(uint *));
		if(!dscreen) FATAL_ERR("realloc", NULL)

		if(resize_width(tid, width, height, dscreen) == -1) return -1;
	} else if(height > descriptors[tid].height) {
		/* in the future this will look something like this:
		 *
		 * if(lines_in_buffer(tid)) {
		 * 	diff = height - descriptors[tid].height;
		 *
		 * 	if(lines_in_buffer(tid) < diff) diff = lines_in_buffer(tid);
		 * } else
		 * 	diff = 0;
		 *
		 * but for now, we're just putting the diff = 0 thing there
		 */

		diff = 0;

		if(resize_width(tid, width, descriptors[tid].height, dscreen) == -1) return -1;

		*screen = dscreen = realloc(dscreen, height * sizeof(uint *));
		if(!dscreen) FATAL_ERR("realloc", NULL)

/*		for(i = diff; i < height; i++)
			dscreen[i] = dscreen[i-diff];*/

		if(diff) {
			memmove(&dscreen[diff], dscreen, descriptors[tid].height * sizeof(uint *));

			if(type == MAINSCREEN)
				descriptors[tid].cursor.y += diff;
		}

		/* probably pop lines off the buffer and put them in here later */
		for(i = 0; i < diff; i++) {
			dscreen[i] = malloc(width * sizeof(uint));
			if(!dscreen[i]) FATAL_ERR("malloc", NULL)

			for(n = 0; n < width; n++)
				dscreen[i][n] = ' ';
		}

		for(i = descriptors[tid].height + diff; i < height; i++) {
			dscreen[i] = malloc(width * sizeof(uint));
			if(!dscreen[i]) FATAL_ERR("malloc", NULL)

			for(n = 0; n < width; n++)
				dscreen[i][n] = ' ';
		}
	} else
		if(resize_width(tid, width, height, dscreen) == -1) return -1;

	return 0;
}

/* a function like this should've been put in POSIX...
 * (minus the libterm-specific stuff, of course)
 */
int tcsetwinsz(int tid) {
	int mfd = fileno(descriptors[tid].pty.master);
	struct winsize size;

	size.ws_row = descriptors[tid].height;
	size.ws_col = descriptors[tid].width;
	size.ws_xpixel = size.ws_ypixel = 0; /* necessary? */

	if(ioctl(mfd, TIOCSWINSZ, &size) == -1) FATAL_ERR("ioctl", "TIOCSWINSZ")
	
	return 0;
}

int ltm_set_window_dimensions(int tid, ushort width, ushort height) {
	char big_changes;
	pid_t pgrp;

	DIE_ON_INVAL_TID(tid)

	if(!width || !height) FIXABLE_LTM_ERR(EINVAL)

	/* if not changing anything, just pretend we did something and exit successfully immediately */
	if(width == descriptors[tid].width && height == descriptors[tid].height) return 0;

	if(height > descriptors[tid].height && 0 /* lines_in_buffer(tid) */)
		big_changes = 1;
	else if(height < descriptors[tid].height && descriptors[tid].cursor.y >= height)
		big_changes = 1;
	else
		big_changes = 0;

	if(set_screen_dimensions(tid, MAINSCREEN, width, height) == -1) return -1;
	/* probably:
	 * set_screen_dimensions(tid, ALTSCREEN, width, height);
	 * in the future
	 * ... or more! (who knows?)
	 *
	 * even though everything else is enabled to support an alternate screen,
	 * I don't want to enable this as it's a fairly expensive operation, and
	 * what's the point of enabling it if it's not actually used yet?
	 */

	descriptors[tid].height = height;
	descriptors[tid].width = width;

	/* since the memory for a descriptor gets zero'd out and MAINSCREEN
	 * evaluates to zero, this should work even when cur_screen hasn't
	 * been explicitly set yet
	 *
	 * is it wrong to make an assumption like this? is it ok to perform
	 * unnecessary operations (explicitly setting cur_screen to MAINSCREEN)
	 * in the name of correctness? I don't know :(
	 */
	if(descriptors[tid].cur_screen == MAINSCREEN)
		descriptors[tid].screen = descriptors[tid].main_screen;
	else if(descriptors[tid].cur_screen == ALTSCREEN)
		descriptors[tid].screen = descriptors[tid].alt_screen;
	/* what to do if cur_screen contains an invalid value??? */

	/* only do this if the pty has been created and the shell has been started! */
	/* checking if pid is set is a quick'n'dirty way or checking whether ltm_term_init finished */
	if(descriptors[tid].pid) {
		if(tcsetwinsz(tid) == -1) return -1;

		pgrp = tcgetpgrp(fileno(descriptors[tid].pty.master));
		if(pgrp == -1) FATAL_ERR("tcgetpgrp", NULL)

		/* not caring whether this fails or not
		 * it can do so if there's currently no
		 * foreground process group or if we don't
		 * have permission to signal some processes
		 * in the process group
		 */
		killpg(pgrp, SIGWINCH);

		/* don't do this before ltm_term_init since the callbacks
		 * aren't guaranteed to be set (and there isn't much point
		 * since there can't be anything on the screen before the
		 * shell starts)
		 */
		if(big_changes)
			cb_refresh_screen(tid);
	}

	return 0;
}
