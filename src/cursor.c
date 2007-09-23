#include <string.h>
#include <stdlib.h>

#include "libterm.h"
#include "cursor.h"
#include "window.h"
#include "bitarr.h"

void cursor_rel_move(int tid, char direction, uint num) {
	if(!num) return;

	switch(direction) {
		case UP:
			if(descs[tid].cursor.y > num)
				descs[tid].cursor.y -= num;
			else if(descs[tid].cursor.y)
				descs[tid].cursor.y = 0;
			else
				return;

			break;
		case DOWN:
			if(descs[tid].cursor.y + num < descs[tid].height)
				descs[tid].cursor.y += num;
			else if(descs[tid].cursor.y != descs[tid].height-1)
				descs[tid].cursor.y = descs[tid].height-1;
			else
				return;
			
			break;
		case LEFT:
			if(descs[tid].cursor.x > num)
				descs[tid].cursor.x -= num;
			else if(descs[tid].cursor.x)
				descs[tid].cursor.x = 0;
			else
				return;

			break;
		case RIGHT:
			if(descs[tid].cursor.x + num < descs[tid].width)
				descs[tid].cursor.x += num;
			else if(descs[tid].cursor.x != descs[tid].width-1)
				descs[tid].cursor.x = descs[tid].width-1;
			else
				return;

			break;
		default:
			return;
	}

	descs[tid].curs_changed = 1;
}

void cursor_abs_move(int tid, char axis, uint num) {
	switch(axis) {
		case X:
			if(num < descs[tid].width && num != descs[tid].cursor.x)
				descs[tid].cursor.x = num;
			else
				return;

			break;
		case Y:
			if(num < descs[tid].height && num != descs[tid].cursor.y)
				descs[tid].cursor.y = num;
			else
				return;

			break;
	}

	descs[tid].curs_changed = 1;
}

void cursor_horiz_tab(int tid) {
	/* don't hardcode 8 here in the future? */
	char dist = 8 - (descs[tid].cursor.x % 8);

	cursor_rel_move(tid, RIGHT, dist);
}

void cursor_vertical_tab(int tid) {
	/* scroll the screen, but only in the main screen */
	if(descs[tid].cursor.y == descs[tid].height-1 && descs[tid].cur_screen == MAINSCREEN)
		scroll_screen(tid);
	else {
		cursor_rel_move(tid, DOWN, 1);

		bitarr_unset_index(descs[tid].wrapped, descs[tid].cursor.y);
	}
}

void cursor_line_break(int tid) {
	cursor_vertical_tab(tid);
	cursor_abs_move(tid, X, 0);
}

void cursor_advance(int tid) {
	if(descs[tid].cursor.x == descs[tid].width-1) {
		cursor_line_break(tid);

		bitarr_set_index(descs[tid].wrapped, descs[tid].cursor.y);
	} else
		cursor_rel_move(tid, RIGHT, 1);
}
