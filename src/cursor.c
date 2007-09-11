#include <string.h>
#include <stdlib.h>

#include "libterm.h"
#include "cursor.h"

void cursor_rel_move(int tid, char direction, uint num) {
	if(!num) return;

	switch(direction) {
		case UP:
			if(descriptors[tid].cursor.y > num)
				descriptors[tid].cursor.y -= num;
			else if(descriptors[tid].cursor.y)
				descriptors[tid].cursor.y = 0;
			else
				return;

			break;
		case DOWN:
			if(descriptors[tid].cursor.y + num < descriptors[tid].height)
				descriptors[tid].cursor.y += num;
			else if(descriptors[tid].cursor.y != descriptors[tid].height-1)
				descriptors[tid].cursor.y = descriptors[tid].height-1;
			else
				return;
			
			break;
		case LEFT:
			if(descriptors[tid].cursor.x > num)
				descriptors[tid].cursor.x -= num;
			else if(descriptors[tid].cursor.x)
				descriptors[tid].cursor.x = 0;
			else
				return;

			break;
		case RIGHT:
			if(descriptors[tid].cursor.x + num < descriptors[tid].width)
				descriptors[tid].cursor.x += num;
			else if(descriptors[tid].cursor.x != descriptors[tid].width-1)
				descriptors[tid].cursor.x = descriptors[tid].width-1;
			else
				return;

			break;
		default:
			return;
	}

	descriptors[tid].curs_changed = 1;
}

void cursor_abs_move(int tid, char axis, uint num) {
	switch(axis) {
		case X:
			if(num < descriptors[tid].width && num != descriptors[tid].cursor.x)
				descriptors[tid].cursor.x = num;
			else
				return;

			break;
		case Y:
			if(num < descriptors[tid].height && num != descriptors[tid].cursor.y)
				descriptors[tid].cursor.y = num;
			else
				return;

			break;
	}

	descriptors[tid].curs_changed = 1;
}

void cursor_line_break(int tid) {
	if(descriptors[tid].cursor.y < descriptors[tid].height-1)
		cursor_rel_move(tid, DOWN, 1);

	cursor_abs_move(tid, X, 0);
}

void cursor_carriage_return(int tid) {
	/* access some sort of bit array which
	 * keeps track of which lines are wrapped
	 * in the future...
	 */

	cursor_abs_move(tid, X, 0);
}

void cursor_advance(int tid) {
	if(descriptors[tid].cursor.x == descriptors[tid].width-1) {
		/* set some sort of bit array which
		 * keeps track of which lines are wrapped
		 * in the future...
		 */

		cursor_line_break(tid, areas, nareas);
	} else
		cursor_rel_move(tid, RIGHT, 1);
}
