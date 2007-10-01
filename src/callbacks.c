#include "libterm.h"
	
int check_callbacks(int tid) {
	if(!descs[tid].cb.update_areas)
		FIXABLE_LTM_ERR(ENOTSUP, "The update_areas callback was not supplied")

	if(!descs[tid].cb.refresh_screen)
		fprintf(dump_dest, "Warning: The refresh_screen callback was not supplied. It will be emulated with update_areas\n");

	if(!descs[tid].cb.scroll_lines)
		fprintf(dump_dest, "Warning: The scroll_lines callback was not supplied. It will be emulated with update_areas\n");

	if(!descs[tid].cb.alert)
		fprintf(dump_dest, "Warning: The alert callback was not supplied. The ASCII bell character will be ignored\n");

	/* more as the ltm_callbacks struct grows... */

	return 0;
}

int cb_update_areas(int tid) {
	struct point * curs;

	if(descs[tid].curs_changed)
		curs = &descs[tid].cursor;
	else
		curs = NULL;

	/* assuming descs[tid].areas has been set up and is null terminated... */
	descs[tid].cb.update_areas(tid, descs[tid].screen, curs, descs[tid].areas);

	descs[tid].curs_changed = 0;

	return 0;
}

int cb_refresh_screen(int tid) {
	struct area * areas[2], area;

	if(descs[tid].cb.refresh_screen)
		descs[tid].cb.refresh_screen(tid, descs[tid].screen);
	else {
		area.start.y = area.start.x = 0;

		area.end.y = descs[tid].height-1;
		area.end.x = descs[tid].width;

		areas[0] = &area;
		areas[1] = NULL;

		descs[tid].areas = areas;

		cb_update_areas(tid);

		descs[tid].areas = 0;
	}

	return 0;
}

int cb_scroll_lines(int tid, uint lines) {
	/*struct area area;*/

	if(descs[tid].cb.scroll_lines)
		descs[tid].cb.scroll_lines(tid, lines);
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
	if(descs[tid].cb.alert)
		descs[tid].cb.alert(tid);

	return 0;
}
