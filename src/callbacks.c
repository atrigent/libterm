#include <string.h>

#include "libterm.h"
#include "screen.h"

int check_callbacks(struct ltm_callbacks *callbacks) {
	int ret = 0;

	if(!callbacks->update_ranges)
		LTM_ERR(ENOTSUP, "The update_ranges callback was not supplied", error);

	if(!callbacks->refresh)
		LTM_ERR(ENOTSUP, "The refresh callback was not supplied", error);

	if(!callbacks->refresh_screen)
		fprintf(dump_dest, "Warning: The refresh_screen callback was not supplied. It will be emulated with update_ranges\n");

	if(!callbacks->scroll_lines)
		fprintf(dump_dest, "Warning: The scroll_lines callback was not supplied. It will be emulated with update_ranges\n");

	if(!callbacks->alert)
		fprintf(dump_dest, "Warning: The alert callback was not supplied. The ASCII bell character will be ignored\n");

#ifdef HAVE_LIBPTHREAD
	if(threading) {
		if(!callbacks->term_exit)
			LTM_ERR(ENOTSUP, "The term_exit callback was not supplied, it is required when threading is on", error);

		if(!callbacks->thread_died)
			LTM_ERR(ENOTSUP, "The thread_died callback was not supplied, it is required when threading is on", error);
	}
#endif

	/* more as the ltm_callbacks struct grows... */

error:
	return ret;
}

int DLLEXPORT ltm_set_callbacks(struct ltm_callbacks *callbacks) {
	int ret = 0;

	CHECK_INITED;

	LOCK_BIG_MUTEX;

	if(check_callbacks(callbacks) == -1) PASS_ERR(after_lock);

	memcpy(&cbs, callbacks, sizeof(struct ltm_callbacks));

after_lock:
	UNLOCK_BIG_MUTEX;
before_anything:
	return ret;
}

int cb_update_ranges(int tid, struct range **ranges) {
	/* assuming ranges is null terminated... */
	cbs.update_ranges(tid, CUR_SCR(tid).matrix, ranges);

	return 0;
}

int cb_update_range(int tid, struct range *range) {
	struct range *ranges[2];

	ranges[0] = range;
	ranges[1] = NULL;

	cb_update_ranges(tid, ranges);

	return 0;
}

int cb_refresh(int tid) {
	struct point *curs;

	if(descs[tid].curs_changed)
		curs = &CUR_SCR(tid).cursor;
	else
		curs = NULL;

	cbs.refresh(tid, curs);

	descs[tid].curs_changed = 0;

	return 0;
}

int cb_refresh_screen(int tid) {
	struct range range;

	if(cbs.refresh_screen)
		cbs.refresh_screen(tid, CUR_SCR(tid).matrix);
	else {
		range.action = ACT_COPY;
		range.val = 0;

		range.leftbound = range.start.y = range.start.x = 0;

		range.end.y = descs[tid].lines-1;
		range.rightbound = range.end.x = descs[tid].cols-1;

		cb_update_range(tid, &range);
	}

	return 0;
}

int cb_scroll_lines(int tid) {
	/*struct range range;*/

	if(cbs.scroll_lines)
		cbs.scroll_lines(tid, descs[tid].lines_scrolled);
	else {
		/*
		range.action = ACT_COPY;
		range.val = 0;

		range.leftbound = range.start.y = range.start.x = 0;

		range.end.y = descs[tid].lines - descs[tid].lines_scrolled - 1;
		range.rightbound = range.end.x = descs[tid].cols;

		cb_update_range(tid, &range);*/

		cb_refresh_screen(tid);
	}

	return 0;
}

int cb_alert(int tid) {
	if(cbs.alert)
		cbs.alert(tid);

	return 0;
}

int cb_term_exit(int tid) {
	cbs.term_exit(tid);

	return 0;
}

int cb_thread_died(struct error_info err) {
	cbs.thread_died(err);

	return 0;
}
