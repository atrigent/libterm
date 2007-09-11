#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "libterm.h"
#include "cursor.h"
#include "callbacks.h"

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
	struct area **areas = 0;
	uint i, nareas = 0;
	char *buf;

	DIE_ON_INVAL_TID(tid)

	if(read_into_outputbuf(tid) == -1) return -1;

	buf = descriptors[tid].outputbuf;

	for(i = 0; i < descriptors[tid].buflen; i++) {
		if(buf[i] > 0x7f) continue;

		if(buf[i] > '~' || buf[i] < ' ') {
			switch(buf[i]) {
				case '\n': /* line break */
					cursor_line_break(tid, areas, &nareas);
					break;
				case '\r': /* carriage return */
					cursor_carriage_return(tid);
					break;
				case '\b': /* backspace */
					cursor_rel_move(tid, LEFT, 1);
					break;
				case '\v': /* vertical tab */
					cursor_vertical_tab(tid, areas, &nareas);
					break;
			}

			continue;
		}

		if(areas == NULL || memcmp(&areas[nareas-1]->end, &descriptors[tid].cursor, sizeof(struct point))) {
			areas = realloc(areas, ++nareas * sizeof(struct area *));
			if(!areas) FATAL_ERR("realloc", NULL)

			areas[nareas-1] = malloc(sizeof(struct area));
			if(!areas[nareas-1]) FATAL_ERR("malloc", NULL)

			areas[nareas-1]->start.y = descriptors[tid].cursor.y;
			areas[nareas-1]->start.x = descriptors[tid].cursor.x;
		}

		descriptors[tid].screen[descriptors[tid].cursor.y][descriptors[tid].cursor.x] = buf[i];

		cursor_advance(tid, areas, &nareas);

		areas[nareas-1]->end.x = descriptors[tid].cursor.x;
		areas[nareas-1]->end.y = descriptors[tid].cursor.y;
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

	areas = realloc(areas, (nareas+1) * sizeof(struct area));
	if(!areas) FATAL_ERR("realloc", NULL)

	areas[nareas] = NULL;

	cb_update_areas(tid, areas);

	for(i = 0; areas[i]; i++) free(areas[i]);
	free(areas);

	return 0;
}
