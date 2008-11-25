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

struct rangeset *record_update(int tid, int sid, char opts) {
	struct rangeset *ret = NULL;
	uint i;

	/* nothing to do */
	if(!opts) return NULL;

	/* no reason to do anything in this case, the app using libterm
	 * doesn't care where the hidden cursor has moved
	 */
	if(opts == UPD_CURS && SCR(tid, sid).curs_invisible)
		return NULL;

	/* check if it's the window screen first because if it's
	 * both the window and the input screen we want stuff
	 * to go directly to the window ranges
	 */
	if(sid == descs[tid].cur_screen) {
		ret = &descs[tid].set;

		if(opts & UPD_SCROLL)
			descs[tid].lines_scrolled++;

		/* the reason that we're un-dirtying the cursor when it is
		 * hidden is that the app using libterm doesn't care
		 * about where the cursor is if it's hidden
		 */
		if(opts & UPD_CURS)
			descs[tid].curs_changed = 1;
		else if(opts & UPD_CURS_INVIS)
			descs[tid].curs_changed = 0;
	} else if(sid == descs[tid].cur_input_screen) {
		for(i = 0; i < descs[tid].scr_nups; i++)
			if(descs[tid].scr_ups[i].sid == sid) break;

		if(i >= descs[tid].scr_nups) {
			descs[tid].scr_ups = realloc(descs[tid].scr_ups, ++descs[tid].scr_nups * sizeof(struct update));
			if(!descs[tid].scr_ups) SYS_ERR_PTR("realloc", NULL, error);

			i = descs[tid].scr_nups-1;

			memset(&descs[tid].scr_ups[i], 0, sizeof(struct update));
			descs[tid].scr_ups[i].sid = sid;
		}

		ret = &descs[tid].scr_ups[i].set;

		if(opts & UPD_SCROLL)
			descs[tid].scr_ups[i].lines_scrolled++;

		/* we need to mark the cursor dirty even when it is being
		 * hidden so that the change will be propagated
		 */
		if(opts & (UPD_CURS | UPD_CURS_INVIS))
			descs[tid].scr_ups[i].curs_changed = 1;
	}

	if(!ret) LTM_ERR_PTR(ESRCH, NULL, error);

error:
	return ret;
}

int screen_scroll(int tid, int sid) {
	struct rangeset *set;
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

	set = record_update(tid, sid, UPD_SCROLL | UPD_GET_SET);
	if(!set) {
		if(ltm_curerr.err_no == ESRCH) return 0;
		else return -1;
	}

	range_shift(set);

	return 0;
}

static int should_be_merged(struct range *range, struct point *start) {
	/* some magic... */
	if(
		(range->start.y == range->end.y &&
			(
				(range->end.y == start->y && range->end.x+1 == start->x) ||
				(range->end.y+1 == start->y && start->x <= range->start.x)
			)
		) ||
		(
			(range->end.y == start->y && range->end.x+1 == start->x && start->x <= range->rightbound) ||
			(range->end.y+1 == start->y && start->x == range->leftbound)
		)
	) return 1;

	return 0;
}

int screen_set_point(int tid, int sid, char action, struct point *pt, uint chr) {
	struct rangeset *curset;

	if(SCR(tid, sid).matrix[pt->y][pt->x] == chr)
		return 0;

	curset = record_update(tid, sid, UPD_GET_SET);
	if(!curset) {
		if(ltm_curerr.err_no == ESRCH) goto set_cell;
		else return -1;
	}

	if(curset->nranges && should_be_merged(TOPRANGE(curset), pt)) {
		if(
			TOPRANGE(curset)->start.y == TOPRANGE(curset)->end.y &&
			TOPRANGE(curset)->end.y+1 == pt->y
		) {
			TOPRANGE(curset)->leftbound = pt->x;
			TOPRANGE(curset)->rightbound = TOPRANGE(curset)->end.x;
		}

		TOPRANGE(curset)->end.x = pt->x;
		TOPRANGE(curset)->end.y = pt->y;
	} else {
		if(range_add(curset) == -1) return -1;

		TOPRANGE(curset)->action = action;
		TOPRANGE(curset)->val = 0;

		TOPRANGE(curset)->leftbound = 0;
		TOPRANGE(curset)->rightbound = SCR(tid, sid).cols-1;

		TOPRANGE(curset)->start.x = pt->x;
		TOPRANGE(curset)->start.y = pt->y;

		TOPRANGE(curset)->end.x = pt->x;
		TOPRANGE(curset)->end.y = pt->y;
	}

set_cell:
	SCR(tid, sid).matrix[pt->y][pt->x] = chr;

	return 0;
}

int screen_inject_update(int tid, int sid, struct range *range) {
	struct rangeset *curset;

	curset = record_update(tid, sid, 0);
	if(!curset) {
		if(ltm_curerr.err_no == ESRCH) return 0;
		else return -1;
	}

	if(range_add(curset) == -1) return -1;

	memcpy(TOPRANGE(curset), range, sizeof(struct range));

	return 0;
}

int screen_clear_range(int tid, int sid, struct range *range) {
	struct point cur;

	if(range->action != ACT_CLEAR) return 0;

	cur.y = range->start.y;
	cur.x = range->start.x;

	while(cur.y <= range->end.y) {
		while(
			(cur.y < range->end.y && cur.x <= range->rightbound) ||
			(cur.y == range->end.y && cur.x <= range->end.x)
		) {
			screen_set_point(tid, sid, ACT_CLEAR, &cur, ' ');

			cur.x++;
		}

		cur.x = range->leftbound;
		cur.y++;
	}

	return 0;
}

int screen_copy_range(int tid, int fromsid, int tosid, struct range *range, struct link *link) {
	struct point srccur, dstcur;

	if(range->action != ACT_COPY) return 0;

	dstcur.x = srccur.x = range->start.x;
	dstcur.y = srccur.y = range->start.y;

	TRANSFORM_PT(dstcur, *link);

	while(srccur.y <= range->end.y) {
		while(
			(srccur.y < range->end.y && srccur.x <= range->rightbound) ||
			(srccur.y == range->end.y && srccur.x <= range->end.x)
		) {
			screen_set_point(tid, tosid, ACT_COPY, &dstcur, SCR(tid, fromsid).matrix[srccur.y][srccur.x]);

			srccur.x++;
			dstcur.x++;
		}

		srccur.x = range->leftbound;
		srccur.y++;

		dstcur.x = range->leftbound + link->origin.x;
		dstcur.y++;
	}

	return 0;
}

int screen_scroll_rect(int tid, int sid, struct range *rect) {
	struct point cur;

	if(
		rect->leftbound != rect->start.x ||
		rect->rightbound != rect->end.x ||
		rect->action != ACT_SCROLL ||
		!rect->val
	) return 0;

	for(cur.y = rect->start.y; cur.y + rect->val <= rect->end.y; cur.y++)
		memcpy(
			&SCR(tid, sid).matrix[cur.y][rect->start.x],
			&SCR(tid, sid).matrix[cur.y + rect->val][rect->start.x],
			sizeof(uint) * (rect->end.x - rect->start.x + 1)
		      );

	for(; cur.y <= rect->end.y; cur.y++)
		for(cur.x = rect->leftbound; cur.x <= rect->rightbound; cur.x++)
			SCR(tid, sid).matrix[cur.y][cur.x] = ' ';

	return screen_inject_update(tid, sid, rect);
}
