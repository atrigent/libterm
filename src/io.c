#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <poll.h>

#include "libterm.h"
#include "cursor.h"
#include "process.h"

int DLLEXPORT ltm_feed_input_to_program(int tid, char * input, uint n) {
	CHECK_INITED;

	LOCK_BIG_MUTEX;

	DIE_ON_INVAL_TID(tid)

	if(fwrite(input, 1, n, descs[tid].pty.master) < n)
		SYS_ERR("fwrite", input);

	UNLOCK_BIG_MUTEX;
	return 0;
}

int DLLEXPORT ltm_simulate_output(int tid, char *input, uint n) {
	CHECK_INITED;

	LOCK_BIG_MUTEX;

	DIE_ON_INVAL_TID(tid)

	if(fwrite(input, 1, n, descs[tid].pty.slave) < n)
		SYS_ERR("fwrite", input);

	UNLOCK_BIG_MUTEX;
	return 0;
}

static int read_into_outputbuf(int tid) {
	uint buflen;
	uchar *buf;

	if(ioctl(fileno(descs[tid].pty.master), FIONREAD, &buflen) == -1)
		SYS_ERR("ioctl", "FIONREAD");

	if(descs[tid].outputbuf) { /* add to the current buffer */
		buf = realloc(descs[tid].outputbuf, descs[tid].buflen + buflen);
		if(!buf) SYS_ERR("realloc", NULL);

		if(fread(buf + descs[tid].buflen, 1, buflen, descs[tid].pty.master) < buflen)
			SYS_ERR("fread", NULL);

		descs[tid].buflen += buflen;
	} else { /* create a new buffer */
		buf = malloc(buflen);
		if(!buf) SYS_ERR("malloc", NULL);

		if(fread(buf, 1, buflen, descs[tid].pty.master) < buflen)
			SYS_ERR("fread", NULL);

		descs[tid].buflen = buflen;
	}

	descs[tid].outputbuf = buf;

	return 0;
}

int DLLEXPORT ltm_process_output(int tid) {
	uchar *buf;
	uint i;

	CHECK_INITED;

#ifdef HAVE_LIBPTHREAD
	if(threading)
		LTM_ERR(ENOTSUP, "Threading is on, so you may not manually tell libterm to process the output");
#endif

	LOCK_BIG_MUTEX;

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
					cursor_abs_move(tid, X, 0);
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

		if(descs[tid].cursor.x == descs[tid].width-1 && descs[tid].curs_prev_not_set)
			cursor_wrap(tid);

		if(descs[tid].areas == NULL || memcmp(&descs[tid].areas[descs[tid].nareas-1]->end, &descs[tid].cursor, sizeof(struct point))) {
			descs[tid].areas = realloc(descs[tid].areas, ++descs[tid].nareas * sizeof(struct area *));
			if(!descs[tid].areas) SYS_ERR("realloc", NULL);

			descs[tid].areas[descs[tid].nareas-1] = malloc(sizeof(struct area));
			if(!descs[tid].areas[descs[tid].nareas-1]) SYS_ERR("malloc", NULL);

			descs[tid].areas[descs[tid].nareas-1]->start.y = descs[tid].cursor.y;
			descs[tid].areas[descs[tid].nareas-1]->start.x = descs[tid].cursor.x;

			descs[tid].areas[descs[tid].nareas-1]->end.y = descs[tid].cursor.y;
			descs[tid].areas[descs[tid].nareas-1]->end.x = descs[tid].cursor.x;
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
		if(!buf) SYS_ERR("malloc", NULL);

		memcpy(buf, &descs[tid].outputbuf[i], descs[tid].buflen);

		free(descs[tid].outputbuf);
		descs[tid].outputbuf = buf;
	} else /* nothing done, exit */
		return 0;

	descs[tid].areas = realloc(descs[tid].areas, (descs[tid].nareas+1) * sizeof(struct area *));
	if(!descs[tid].areas) SYS_ERR("realloc", NULL);

	descs[tid].areas[descs[tid].nareas] = NULL;

	cb_update_areas(tid);

	for(i = 0; descs[tid].areas[i]; i++) free(descs[tid].areas[i]);
	free(descs[tid].areas);

	descs[tid].areas = NULL;
	descs[tid].nareas = 0;

	UNLOCK_BIG_MUTEX;
	return 0;
}

#ifdef HAVE_LIBPTHREAD
/* use poll because there's no limit on the number of fds to watch */
void *watch_for_events() {
	int i, n, nfds, ret, newtid;
	struct pollfd *fds;
	char code, running = 1;

	LOCK_BIG_MUTEX_THR;

	/* start out with just the pipe fd... */
	fds = malloc(sizeof(struct pollfd));
	if(!fds) SYS_ERR_THR("malloc", NULL);

	fds[0].fd = fileno(pipefiles[0]);
	fds[0].events = POLLIN;

	nfds = 1;

	while(running) {
		UNLOCK_BIG_MUTEX_THR;

		ret = poll(fds, nfds, -1);

		if(!ret || (ret == -1 && errno == EINTR)) continue;
		else if(ret == -1) SYS_ERR_THR("poll", NULL);

		LOCK_BIG_MUTEX_THR;

		/* Go backwards to make sure that all unhandled stuff in a terminal that just exited
		 * is handled before it is removed from the struct pollfd array. Also, this doesn't
		 * mess with the nfds variable or the struct pollfd array when the main part of the
		 * loop is running. Everybody wins!
		 */
		for(i = nfds-1; i >= 0; i--)
			if(fds[i].revents) {
				if(!i) {
					/* got a new term, delete term, or exit notification... */
					if(fread(&code, 1, sizeof(char), pipefiles[0]) < sizeof(char))
						SYS_ERR_THR("fread", NULL);

					if(code == EXIT_THREAD) {
						running = 0;
						break;
					}

					if(fread(&newtid, 1, sizeof(int), pipefiles[0]) < sizeof(int))
						SYS_ERR_THR("fread", NULL);

					switch(code) {
						case NEW_TERM:
							fds = realloc(fds, (++nfds) * sizeof(struct pollfd));
							if(!fds) SYS_ERR_THR("realloc", NULL);

							fds[nfds-1].fd = fileno(descs[newtid].pty.master);
							fds[nfds-1].events = POLLIN;
							break;
						case DEL_TERM:
							/* find the one which will be deleted... */
							for(n = 1; n < nfds; n++)
								if(fds[n].fd == fileno(descs[newtid].pty.master)) {
									/* remove from the array */
									memmove(&fds[n], &fds[n+1], (nfds-n-1) * sizeof(struct pollfd));

									fds = realloc(fds, (--nfds) * sizeof(struct pollfd));
									if(!fds) SYS_ERR_THR("realloc", NULL);

									break;
								}

							cb_term_exit(newtid);

							break;
					}

				}
				else for(n = 0; n < next_tid; n++)
					if(descs[n].allocated && fds[i].fd == fileno(descs[n].pty.master)) {
						if(ltm_process_output(n) == -1) return NULL;
						break;
					}

				fds[i].revents = 0;
			}
	}

	free(fds);

	UNLOCK_BIG_MUTEX_THR;
	return (void*)1;
}
#endif
