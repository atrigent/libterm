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

	if(ioctl(fileno(descs[tid].pty.master), FIONREAD, &buflen) == -1)
		FATAL_ERR("ioctl", "FIONREAD")

	if(descs[tid].outputbuf) { /* add to the current buffer */
		buf = realloc(descs[tid].outputbuf, descs[tid].buflen + buflen);
		if(!buf) FATAL_ERR("realloc", NULL)

		if(fread(buf + descs[tid].buflen, 1, buflen, descs[tid].pty.master) < buflen)
			FATAL_ERR("fread", NULL)

		descs[tid].buflen += buflen;
	} else { /* create a new buffer */
		buf = malloc(buflen);
		if(!buf) FATAL_ERR("malloc", NULL)

		if(fread(buf, 1, buflen, descs[tid].pty.master) < buflen)
			FATAL_ERR("fread", NULL)

		descs[tid].buflen = buflen;
	}

	descs[tid].outputbuf = buf;

	return 0;
}

int ltm_process_output(int tid) {
	struct area **areas = 0;
	uint i, nareas = 0;
	char *buf;

	DIE_ON_INVAL_TID(tid)

	if(read_into_outputbuf(tid) == -1) return -1;

	buf = descs[tid].outputbuf;

	for(i = 0; i < descs[tid].buflen; i++) {
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

		if(areas == NULL || memcmp(&areas[nareas-1]->end, &descs[tid].cursor, sizeof(struct point))) {
			areas = realloc(areas, ++nareas * sizeof(struct area *));
			if(!areas) FATAL_ERR("realloc", NULL)

			areas[nareas-1] = malloc(sizeof(struct area));
			if(!areas[nareas-1]) FATAL_ERR("malloc", NULL)

			areas[nareas-1]->start.y = descs[tid].cursor.y;
			areas[nareas-1]->start.x = descs[tid].cursor.x;
		}

		descs[tid].screen[descs[tid].cursor.y][descs[tid].cursor.x] = buf[i];

		cursor_advance(tid, areas, &nareas);

		areas[nareas-1]->end.x = descs[tid].cursor.x;
		areas[nareas-1]->end.y = descs[tid].cursor.y;
	}

	if(i == descs[tid].buflen) {
		/* finished processing, free the buffer */

		free(descs[tid].outputbuf);
		descs[tid].outputbuf = NULL;
		descs[tid].buflen = 0;
	} else if(i) {
		/* still unprocessed stuff */

		descs[tid].buflen -= i;

		buf = malloc(descs[tid].buflen);
		if(!buf) FATAL_ERR("malloc", NULL)

		memcpy(buf, &descs[tid].outputbuf[i], descs[tid].buflen);

		free(descs[tid].outputbuf);
		descs[tid].outputbuf = buf;
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
