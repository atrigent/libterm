#include <stdlib.h>
#include <string.h>

#include "libterm.h"
#include "bitarr.h"
#include "screen.h"
#include "cursor.h"
#include "idarr.h"

int screen_alloc(int tid, char autoscroll, ushort lines, ushort cols) {
	uint **screen;
	uint i, n;
	int ret;

	ret = IDARR_ID_ALLOC(descs[tid].screens, descs[tid].next_sid);
	if(ret == -1) return -1;

	screen = SCR(tid, ret).matrix = malloc(lines * sizeof(SCR(tid, ret).matrix[0]));
	if(!screen) SYS_ERR("malloc", NULL, error);

	for(i = 0; i < lines; i++) {
		screen[i] = malloc(cols * sizeof(screen[0][0]));
		if(!screen[i]) SYS_ERR("malloc", NULL, error);

		for(n = 0; n < cols; n++)
			screen[i][n] = ' ';
	}

	SCR(tid, ret).lines = lines;
	SCR(tid, ret).cols = cols;
	SCR(tid, ret).autoscroll = autoscroll;

	if(bitarr_resize(&SCR(tid, ret).wrapped, 0, lines) == -1) return -1;

error:
	return ret;
}

int screen_make_current(int tid, int sid) {
	int ret = 0;

	if(
		SCR(tid, sid).lines != descs[tid].lines ||
		SCR(tid, sid).cols != descs[tid].cols
	  ) LTM_ERR(EINVAL, "Screen does not have the same dimensions as the window", error);

	descs[tid].cur_screen = sid;

error:
	return ret;
}

void screen_give_input_focus(int tid, int sid) {
	/* does any checking need to be done here...? */

	descs[tid].cur_input_screen = sid;
}

void screen_set_autoscroll(int tid, int sid, char autoscroll) {
	SCR(tid, sid).autoscroll = autoscroll;
}

static int resize_cols(int tid, int sid, ushort lines, ushort cols) {
	uint **screen;
	ushort i, n;
	int ret = 0;

	screen = SCR(tid, sid).matrix;

	if(SCR(tid, sid).cols != cols)
		for(i = 0; i < lines; i++) {
			screen[i] = realloc(screen[i], cols * sizeof(screen[0][0]));
			if(!screen[i]) SYS_ERR("realloc", NULL, error);

			if(cols > SCR(tid, sid).cols)
				for(n = SCR(tid, sid).cols; n < cols; n++)
					screen[i][n] = ' ';
		}

error:
	return ret;
}

int screen_set_dimensions(int tid, int sid, ushort lines, ushort cols) {
	uint **dscreen, ***screen;
	ushort i, diff, n;
	int ret = 0;

	screen = &SCR(tid, sid).matrix;
	dscreen = *screen;

	if(lines < SCR(tid, sid).lines) {
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

		if(SCR(tid, sid).cursor.y >= lines)
			diff = SCR(tid, sid).cursor.y - (lines-1);
		else
			diff = 0;

		for(i = 0; i < diff; i++)
			/* probably push lines into the buffer later */
			free(dscreen[i]);

		for(i = lines + diff; i < SCR(tid, sid).lines; i++)
			free(dscreen[i]);

/*		for(i = 0; i < lines; i++)
			dscreen[i] = dscreen[i+diff];*/

		if(diff) {
			memmove(dscreen, &dscreen[diff], lines * sizeof(dscreen[0]));

			if(cursor_rel_move(tid, sid, UP, diff) == -1) return -1;

			bitarr_shift_left(SCR(tid, sid).wrapped, SCR(tid, sid).lines, diff);
		}

		*screen = dscreen = realloc(dscreen, lines * sizeof(dscreen[0]));
		if(!dscreen) SYS_ERR("realloc", NULL, error);

		if(bitarr_resize(&SCR(tid, sid).wrapped, SCR(tid, sid).lines, lines) == -1) return -1;

		if(resize_cols(tid, sid, lines, cols) == -1) return -1;
	} else if(lines > SCR(tid, sid).lines) {
		/* in the future this will look something like this:
		 *
		 * if(lines_in_buffer(tid)) {
		 * 	diff = lines - SCR(tid, sid).lines;
		 *
		 * 	if(lines_in_buffer(tid) < diff) diff = lines_in_buffer(tid);
		 * } else
		 * 	diff = 0;
		 *
		 * but for now, we're just putting the diff = 0 thing there
		 */

		diff = 0;

		if(resize_cols(tid, sid, SCR(tid, sid).lines, cols) == -1) return -1;

		*screen = dscreen = realloc(dscreen, lines * sizeof(dscreen[0]));
		if(!dscreen) SYS_ERR("realloc", NULL, error);

		if(bitarr_resize(&SCR(tid, sid).wrapped, SCR(tid, sid).lines, lines) == -1) return -1;

/*		for(i = diff; i < lines; i++)
			dscreen[i] = dscreen[i-diff];*/

		if(diff) {
			memmove(&dscreen[diff], dscreen, SCR(tid, sid).lines * sizeof(dscreen[0]));

			if(cursor_rel_move(tid, sid, DOWN, diff) == -1) return -1;

			bitarr_shift_right(SCR(tid, sid).wrapped, SCR(tid, sid).lines, diff);
		}

		/* probably pop lines off the buffer and put them in here later */
		for(i = 0; i < diff; i++) {
			dscreen[i] = malloc(cols * sizeof(dscreen[0][0]));
			if(!dscreen[i]) SYS_ERR("malloc", NULL, error);

			for(n = 0; n < cols; n++)
				dscreen[i][n] = ' ';
		}

		for(i = SCR(tid, sid).lines + diff; i < lines; i++) {
			dscreen[i] = malloc(cols * sizeof(dscreen[0][0]));
			if(!dscreen[i]) SYS_ERR("malloc", NULL, error);

			for(n = 0; n < cols; n++)
				dscreen[i][n] = ' ';
		}
	} else
		if(resize_cols(tid, sid, lines, cols) == -1) return -1;

	SCR(tid, sid).lines = lines;
	SCR(tid, sid).cols = cols;

error:
	return ret;
}

int screen_scroll(int tid, int sid) {
	uint i, *linesave;

	/* push this line into the buffer later... */
	linesave = SCR(tid, sid).matrix[0];
	for(i = 0; i < SCR(tid, sid).cols; i++)
		linesave[i] = ' ';

	memmove(
		SCR(tid, sid).matrix,
		&SCR(tid, sid).matrix[1],
		sizeof(SCR(tid, sid).matrix[0]) * (SCR(tid, sid).lines-1)
	);

	SCR(tid, sid).matrix[SCR(tid, sid).lines-1] = linesave;

	bitarr_shift_left(SCR(tid, sid).wrapped, SCR(tid, sid).lines, 1);

	if(sid == descs[tid].cur_screen) {
		range_shift(&descs[tid].set);
		descs[tid].lines_scrolled++;
	}

	return 0;
}
