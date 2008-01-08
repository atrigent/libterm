#include <string.h>
#include <stdlib.h>

#include "libterm.h"
#include "cursor.h"
#include "window.h"
#include "bitarr.h"

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

void cursor_rel_move(int tid, char direction, uint num) {
	if(!num) return;

	switch(direction) {
		case UP:
			if(descs[tid].cursor.y >= num)
				cursor_abs_move(tid, Y, descs[tid].cursor.y - num);
			else
				cursor_abs_move(tid, Y, 0);

			break;
		case DOWN:
			if(descs[tid].cursor.y + num < descs[tid].height)
				cursor_abs_move(tid, Y, descs[tid].cursor.y + num);
			else
				cursor_abs_move(tid, Y, descs[tid].height-1);

			break;
		case LEFT:
			if(descs[tid].cursor.x >= num)
				cursor_abs_move(tid, X, descs[tid].cursor.x - num);
			else
				cursor_abs_move(tid, X, 0);

			break;
		case RIGHT:
			if(descs[tid].cursor.x + num < descs[tid].width)
				cursor_abs_move(tid, X, descs[tid].cursor.x + num);
			else
				cursor_abs_move(tid, X, descs[tid].width-1);

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
	if(descs[tid].cursor.y == descs[tid].height-1 && descs[tid].cur_screen == MAINSCREEN)
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
	if(descs[tid].cursor.x == descs[tid].width-1) {
		cursor_wrap(tid);
	} else
		cursor_rel_move(tid, RIGHT, 1);
}
