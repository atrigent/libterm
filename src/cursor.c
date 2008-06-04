#include <string.h>
#include <stdlib.h>

#include "libterm.h"
#include "cursor.h"
#include "screen.h"
#include "bitarr.h"

void cursor_abs_move(int tid, char axis, ushort num) {
	switch(axis) {
		case X:
			if(num == descs[tid].cursor.x)
				goto not_moved;

			if(num < descs[tid].cols)
				descs[tid].cursor.x = num;
			else
				descs[tid].cursor.x = descs[tid].cols-1;

			break;
		case Y:
			if(num == descs[tid].cursor.y)
				goto not_moved;

			if(num < descs[tid].lines)
				descs[tid].cursor.y = num;
			else
				descs[tid].cursor.y = descs[tid].lines-1;

			break;
	}

	descs[tid].curs_changed = 1;
not_moved:
	descs[tid].curs_prev_not_set = 0;
}

void cursor_rel_move(int tid, char direction, ushort num) {
	if(!num) return;

	switch(direction) {
		case UP:
			cursor_abs_move(tid, Y, num <= descs[tid].cursor.y ? descs[tid].cursor.y - num : 0);
			break;
		case DOWN:
			cursor_abs_move(tid, Y, descs[tid].cursor.y + num);
			break;
		case LEFT:
			cursor_abs_move(tid, X, num <= descs[tid].cursor.x ? descs[tid].cursor.x - num : 0);
			break;
		case RIGHT:
			cursor_abs_move(tid, X, descs[tid].cursor.x + num);
			break;
	}
}

void cursor_horiz_tab(int tid) {
	/* don't hardcode 8 here in the future? */
	char dist = 8 - (descs[tid].cursor.x % 8);

	cursor_rel_move(tid, RIGHT, dist);
}

void cursor_down(int tid) {
	/* scroll the screen, but only in the main screen */
	if(descs[tid].cursor.y == descs[tid].lines-1 && descs[tid].cur_screen == MAINSCREEN)
		scroll_screen(tid);
	else
		cursor_rel_move(tid, DOWN, 1);
}

void cursor_vertical_tab(int tid) {
	cursor_down(tid);
	bitarr_unset_index(descs[tid].wrapped, descs[tid].cursor.y);
}

void cursor_line_break(int tid) {
	cursor_vertical_tab(tid);
	cursor_abs_move(tid, X, 0);
}

void cursor_wrap(int tid) {
	cursor_down(tid);
	bitarr_set_index(descs[tid].wrapped, descs[tid].cursor.y);
	cursor_abs_move(tid, X, 0);
}

void cursor_advance(int tid) {
	if(descs[tid].cursor.x == descs[tid].cols-1) {
		if(!descs[tid].curs_prev_not_set) {
			descs[tid].curs_prev_not_set = 1;
			return;
		}

		cursor_wrap(tid);
	} else
		cursor_rel_move(tid, RIGHT, 1);
}
