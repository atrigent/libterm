#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "libterm.h"

/* checks if second is one point ahead of first */
int is_one_ahead(int tid, struct point * first, struct point * second) {
	if(
		(first->x+1 == second->x && first->y == second->y) ||
		(first->x == descriptors[tid].width-1 && second->x == 0 && first->y+1 == second->y)
	  ) return 1;
	
	return 0;
}

int read_into_outputbuf(int tid) {
	uint buflen;
	char *buf;

	if(ioctl(fileno(descriptors[tid].pty.master), FIONREAD, &buflen) == -1)
		FATAL_ERR("ioctl", "FIONREAD")

	if(descriptors[tid].outputbuf) { /* add to the current buffer */
		buf = realloc(descriptors[tid].outputbuf, descriptors[tid].buflen + buflen);
		if(!buf) FATAL_ERR("realloc", NULL)

		if(fread(buf + descriptors[tid].buflen, 1, buflen, descriptors[tid].pty.master) < buflen)
			FATAL_ERR("fread", NULL)

		descriptors[tid].buflen += buflen;
	} else { /* create a new buffer */
		buf = malloc(buflen);
		if(!buf) FATAL_ERR("malloc", NULL)

		if(fread(buf, 1, buflen, descriptors[tid].pty.master) < buflen)
			FATAL_ERR("fread", NULL)

		descriptors[tid].buflen = buflen;
	}

	descriptors[tid].outputbuf = buf;

	return 0;
}

int ltm_process_output(int tid) {
	struct area * areas = 0;
	struct point * curs;
	uint i, nareas = 0;
	char *buf;

	DIE_ON_INVAL_TID(tid)

	if(read_into_outputbuf(tid) == -1) return -1;

	buf = descriptors[tid].outputbuf;

	for(i = 0; i < descriptors[tid].buflen; i++) {
		if(buf[i] > 0x7f) continue;

		if(buf[i] == '\n') { /* line break */
			descriptors[tid].cursor.y++;
			descriptors[tid].cursor.x = 0;
			descriptors[tid].curs_changed = 1;
			continue;
		} else if(buf[i] == '\r') { /* carriage return */
			descriptors[tid].cursor.x = 0;
			descriptors[tid].curs_changed = 1;
			continue;
		} else if(buf[i] > '~' || buf[i] < ' ') continue;

		if(areas == NULL || memcmp(&areas[nareas-1].end, &descriptors[tid].cursor, sizeof(struct point))) {
			areas = realloc(areas, ++nareas * sizeof(struct area));
			if(!areas) FATAL_ERR("realloc", NULL)

			areas[nareas-1].start.y = areas[nareas-1].end.y = descriptors[tid].cursor.y;
			areas[nareas-1].start.x = areas[nareas-1].end.x = descriptors[tid].cursor.x;
		}

		descriptors[tid].screen[descriptors[tid].cursor.y][descriptors[tid].cursor.x] = buf[i];

		if(descriptors[tid].cursor.x == descriptors[tid].width-1) {
			descriptors[tid].cursor.y++;
			descriptors[tid].cursor.x = 0;
			descriptors[tid].curs_changed = 1;

			areas[nareas-1].end.y++;
			areas[nareas-1].end.x = 0;
		} else {
			descriptors[tid].cursor.x++;
			descriptors[tid].curs_changed = 1;

			areas[nareas-1].end.x++;
		}
	}

	if(i == descriptors[tid].buflen) {
		/* finished processing, free the buffer */

		free(descriptors[tid].outputbuf);
		descriptors[tid].outputbuf = NULL;
		descriptors[tid].buflen = 0;
	} else if(i) {
		/* still unprocessed stuff */

		descriptors[tid].buflen -= i;

		buf = malloc(descriptors[tid].buflen);
		if(!buf) FATAL_ERR("malloc", NULL)

		memcpy(buf, &descriptors[tid].outputbuf[i], descriptors[tid].buflen);

		free(descriptors[tid].outputbuf);
		descriptors[tid].outputbuf = buf;
	} else /* nothing done, exit */
		return 0;

	if(descriptors[tid].curs_changed)
		curs = &descriptors[tid].cursor;
	else
		curs = NULL;

	descriptors[tid].cb.update_areas(tid, descriptors[tid].screen, curs, areas, nareas);

	free(areas);

	return 0;
}
