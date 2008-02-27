#include <string.h>

#include "libterm.h"

int check_callbacks(struct ltm_callbacks *callbacks) {
	int ret = 0;

	if(!callbacks->update_areas)
		LTM_ERR(ENOTSUP, "The update_areas callback was not supplied", error);

	if(!callbacks->refresh_screen)
		fprintf(dump_dest, "Warning: The refresh_screen callback was not supplied. It will be emulated with update_areas\n");

	if(!callbacks->scroll_lines)
		fprintf(dump_dest, "Warning: The scroll_lines callback was not supplied. It will be emulated with update_areas\n");

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

int cb_update_areas(int tid) {
	struct point * curs;

	if(descs[tid].curs_changed)
		curs = &descs[tid].cursor;
	else
		curs = NULL;

	/* assuming descs[tid].areas has been set up and is null terminated... */
	cbs.update_areas(tid, descs[tid].screen, curs, descs[tid].areas);

	descs[tid].curs_changed = 0;

	return 0;
}

int cb_refresh_screen(int tid) {
	struct area *new_areas[2], **areas_save, area;

	if(cbs.refresh_screen)
		cbs.refresh_screen(tid, descs[tid].screen);
	else {
		areas_save = descs[tid].areas;

		area.start.y = area.start.x = 0;

		area.end.y = descs[tid].height-1;
		area.end.x = descs[tid].width;

		new_areas[0] = &area;
		new_areas[1] = NULL;

		descs[tid].areas = new_areas;

		cb_update_areas(tid);

		descs[tid].areas = areas_save;
	}

	return 0;
}

int cb_scroll_lines(int tid, uint lines) {
	/*struct area area;*/

	if(cbs.scroll_lines)
		cbs.scroll_lines(tid, lines);
	else {
		/*area.start.y = area.start.x = 0;

		area.end.y = descs[tid].height - lines - 1;
		area.end.x = descs[tid].width;

		descs[tid].cb.update_areas(tid, descs[tid].screen, NULL, &area, 1);*/

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
