#include <string.h>
#include <stdlib.h>

#include "libterm.h"
#include "cursor.h"
#include "screen.h"
#include "bitarr.h"

int cursor_visibility(int tid, int sid, char visibility) {
	if(SCR(tid, sid).curs_invisible != !visibility) {
		SCR(tid, sid).curs_invisible = !visibility;

		if(sid == descs[tid].cur_screen)
			descs[tid].curs_changed = !SCR(tid, sid).curs_invisible;
	}

	return 0;
}

int cursor_abs_move(int tid, int sid, char axis, ushort num) {
	int ret = 0;

	switch(axis) {
		case X:
			if(num == SCR(tid, sid).cursor.x)
				goto not_moved;

			if(num < SCR(tid, sid).cols)
				SCR(tid, sid).cursor.x = num;
			else
				SCR(tid, sid).cursor.x = SCR(tid, sid).cols-1;

			break;
		case Y:
			if(num == SCR(tid, sid).cursor.y)
				goto not_moved;

			if(num < SCR(tid, sid).lines)
				SCR(tid, sid).cursor.y = num;
			else
				SCR(tid, sid).cursor.y = SCR(tid, sid).lines-1;

			break;
		default:
			LTM_ERR(EINVAL, "Invalid axis", error);
	}

	if(!SCR(tid, sid).curs_invisible)
		if(sid == descs[tid].cur_screen)
			descs[tid].curs_changed = 1;

not_moved:
	SCR(tid, sid).curs_prev_not_set = 0;

error:
	return ret;
}

int cursor_rel_move(int tid, int sid, char direction, ushort num) {
	int ret = 0;

	if(!num) return 0;

	switch(direction) {
		case UP:
			return cursor_abs_move(tid, sid, Y, num <= SCR(tid, sid).cursor.y ? SCR(tid, sid).cursor.y - num : 0);
		case DOWN:
			return cursor_abs_move(tid, sid, Y, SCR(tid, sid).cursor.y + num);
		case LEFT:
			return cursor_abs_move(tid, sid, X, num <= SCR(tid, sid).cursor.x ? SCR(tid, sid).cursor.x - num : 0);
		case RIGHT:
			return cursor_abs_move(tid, sid, X, SCR(tid, sid).cursor.x + num);
		default:
			LTM_ERR(EINVAL, "Invalid direction", error);
	}

error:
	return ret;
}

int cursor_horiz_tab(int tid, int sid) {
	/* don't hardcode 8 here in the future? */
	char dist = 8 - (SCR(tid, sid).cursor.x % 8);

	return cursor_rel_move(tid, sid, RIGHT, dist);
}

int cursor_down(int tid, int sid) {
	if(SCR(tid, sid).cursor.y == SCR(tid, sid).lines-1 && SCR(tid, sid).autoscroll)
		return screen_scroll(tid, sid);
	else
		return cursor_rel_move(tid, sid, DOWN, 1);
}

int cursor_vertical_tab(int tid, int sid) {
	if(cursor_down(tid, sid) == -1) return -1;
	bitarr_unset_index(SCR(tid, sid).wrapped, SCR(tid, sid).cursor.y);

	return 0;
}

int cursor_line_break(int tid, int sid) {
	if(cursor_vertical_tab(tid, sid) == -1) return -1;
	if(cursor_abs_move(tid, sid, X, 0) == -1) return -1;

	return 0;
}

int cursor_wrap(int tid, int sid) {
	if(cursor_down(tid, sid) == -1) return -1;
	bitarr_set_index(SCR(tid, sid).wrapped, SCR(tid, sid).cursor.y);
	if(cursor_abs_move(tid, sid, X, 0) == -1) return -1;

	return 0;
}

int cursor_advance(int tid, int sid) {
	if(SCR(tid, sid).cursor.x == SCR(tid, sid).cols-1) {
		if(!SCR(tid, sid).curs_prev_not_set) {
			SCR(tid, sid).curs_prev_not_set = 1;
			return 0;
		}

		return cursor_wrap(tid, sid);
	} else
		return cursor_rel_move(tid, sid, RIGHT, 1);
}
