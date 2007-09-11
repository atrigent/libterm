#include "libterm.h"
	
int check_callbacks(int tid) {
	if(!descriptors[tid].cb.update_areas)
		FIXABLE_LTM_ERR(ENOTSUP)

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
