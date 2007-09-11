#include "libterm.h"
	
int check_callbacks(int tid) {
	if(!descriptors[tid].cb.update_areas)
		FIXABLE_LTM_ERR(ENOTSUP)

	if(!descriptors[tid].cb.refresh_screen)
		fprintf(dump_dest, "Warning: The refresh_screen callback was not supplied. It will be emulated with update_areas\n");

	if(!descriptors[tid].cb.scroll_lines)
		fprintf(dump_dest, "Warning: The scroll_lines callback was not supplied. It will be emulated with update_areas\n");

	/* more as the ltm_callbacks struct grows... */

	return 0;
}

int cb_update_areas(int tid, struct area ** areas) {
	struct point * curs;

	if(descriptors[tid].curs_changed)
		curs = &descriptors[tid].cursor;
	else
		curs = NULL;

	descriptors[tid].cb.update_areas(tid, descriptors[tid].screen, curs, areas);

	descriptors[tid].curs_changed = 0;

	return 0;
}

int cb_refresh_screen(int tid) {
	struct area * areas[2], area;

	if(descriptors[tid].cb.refresh_screen)
		descriptors[tid].cb.refresh_screen(tid, descriptors[tid].screen);
	else {
		area.start.y = area.start.x = 0;

		area.end.y = descriptors[tid].height-1;
		area.end.x = descriptors[tid].width;

		areas[0] = &area;
		areas[1] = NULL;

		cb_update_areas(tid, areas);
	}

	return 0;
}

int cb_scroll_lines(int tid, uint lines) {
	/*struct area area;*/

	if(descriptors[tid].cb.scroll_lines)
		descriptors[tid].cb.scroll_lines(tid, lines);
	else {
		/*area.start.y = area.start.x = 0;

		area.end.y = descriptors[tid].height - lines - 1;
		area.end.x = descriptors[tid].width;

		descriptors[tid].cb.update_areas(tid, descriptors[tid].screen, NULL, &area, 1);*/

		cb_refresh_screen(tid);
	}

	return 0;
}
