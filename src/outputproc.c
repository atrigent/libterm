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
	char *buf;
	uint i;

	DIE_ON_INVAL_TID(tid)

	if(read_into_outputbuf(tid) == -1) return -1;

	buf = descs[tid].outputbuf;

	for(i = 0; i < descs[tid].buflen; i++) {
		if(buf[i] > 0x7f) continue;

		if(buf[i] > '~' || buf[i] < ' ') {
			switch(buf[i]) {
				case '\n': /* line break */
					cursor_line_break(tid);
					break;
				case '\r': /* carriage return */
					cursor_carriage_return(tid);
					break;
				case '\b': /* backspace */
					cursor_rel_move(tid, LEFT, 1);
					break;
				case '\v': /* vertical tab */
					cursor_vertical_tab(tid);
					break;
				case '\t': /* horizontal tab */
					cursor_horiz_tab(tid);
					break;
				case '\a': /* bell */
					cb_alert(tid);
					break;
			}

			continue;
		}

		if(descs[tid].areas == NULL || memcmp(&descs[tid].areas[descs[tid].nareas-1]->end, &descs[tid].cursor, sizeof(struct point))) {
			descs[tid].areas = realloc(descs[tid].areas, ++descs[tid].nareas * sizeof(struct area *));
			if(!descs[tid].areas) FATAL_ERR("realloc", NULL)

			descs[tid].areas[descs[tid].nareas-1] = malloc(sizeof(struct area));
			if(!descs[tid].areas[descs[tid].nareas-1]) FATAL_ERR("malloc", NULL)

			descs[tid].areas[descs[tid].nareas-1]->start.y = descs[tid].cursor.y;
			descs[tid].areas[descs[tid].nareas-1]->start.x = descs[tid].cursor.x;
		}

		descs[tid].screen[descs[tid].cursor.y][descs[tid].cursor.x] = buf[i];

		cursor_advance(tid);

		descs[tid].areas[descs[tid].nareas-1]->end.x = descs[tid].cursor.x;
		descs[tid].areas[descs[tid].nareas-1]->end.y = descs[tid].cursor.y;
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

	descs[tid].areas = realloc(descs[tid].areas, (descs[tid].nareas+1) * sizeof(struct area));
	if(!descs[tid].areas) FATAL_ERR("realloc", NULL)

	descs[tid].areas[descs[tid].nareas] = NULL;

	cb_update_areas(tid);

	for(i = 0; descs[tid].areas[i]; i++) free(descs[tid].areas[i]);
	free(descs[tid].areas);

	descs[tid].areas = NULL;
	descs[tid].nareas = 0;

	return 0;
}
