#include <string.h>
#include <stdlib.h>

#include "libterm.h"
#include "cursor.h"
#include "screen.h"
#include "bitarr.h"

int cursor_abs_move(int tid, char axis, ushort num) {
	int ret = 0;

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
		default:
			LTM_ERR(EINVAL, "Invalid axis", error);
	}

	descs[tid].curs_changed = 1;
not_moved:
	descs[tid].curs_prev_not_set = 0;

error:
	return ret;
}

int cursor_rel_move(int tid, char direction, ushort num) {
	int ret = 0;

	if(!num) return 0;

	switch(direction) {
		case UP:
			return cursor_abs_move(tid, Y, num <= descs[tid].cursor.y ? descs[tid].cursor.y - num : 0);
		case DOWN:
			return cursor_abs_move(tid, Y, descs[tid].cursor.y + num);
		case LEFT:
			return cursor_abs_move(tid, X, num <= descs[tid].cursor.x ? descs[tid].cursor.x - num : 0);
		case RIGHT:
			return cursor_abs_move(tid, X, descs[tid].cursor.x + num);
		default:
			LTM_ERR(EINVAL, "Invalid direction", error);
	}

error:
	return ret;
}

int cursor_horiz_tab(int tid) {
	/* don't hardcode 8 here in the future? */
	char dist = 8 - (descs[tid].cursor.x % 8);

	return cursor_rel_move(tid, RIGHT, dist);
}

int cursor_down(int tid) {
	/* scroll the screen, but only in the main screen */
	if(descs[tid].cursor.y == descs[tid].lines-1 && descs[tid].cur_screen == MAINSCREEN)
		screen_scroll(tid);
	else
		return cursor_rel_move(tid, DOWN, 1);

	return 0;
}

int cursor_vertical_tab(int tid) {
	if(cursor_down(tid) == -1) return -1;
	bitarr_unset_index(descs[tid].wrapped, descs[tid].cursor.y);

	return 0;
}

int cursor_line_break(int tid) {
	if(cursor_vertical_tab(tid) == -1) return -1;
	if(cursor_abs_move(tid, X, 0) == -1) return -1;

	return 0;
}

int cursor_wrap(int tid) {
	if(cursor_down(tid) == -1) return -1;
	bitarr_set_index(descs[tid].wrapped, descs[tid].cursor.y);
	if(cursor_abs_move(tid, X, 0) == -1) return -1;

	return 0;
}

int cursor_advance(int tid) {
	if(descs[tid].cursor.x == descs[tid].cols-1) {
		if(!descs[tid].curs_prev_not_set) {
			descs[tid].curs_prev_not_set = 1;
			return 0;
		}

		return cursor_wrap(tid);
	} else
		return cursor_rel_move(tid, RIGHT, 1);
}
