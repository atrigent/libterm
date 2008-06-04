#include <stdlib.h>
#include <string.h>

#include "libterm.h"
#include "bitarr.h"
#include "cursor.h"

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

int screen_set_dimensions(int tid, char type, ushort lines, ushort cols) {
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

int screen_scroll(int tid) {
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
