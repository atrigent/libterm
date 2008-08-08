#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <poll.h>

#include "libterm.h"
#include "cursor.h"
#include "process.h"

int DLLEXPORT ltm_feed_input_to_program(int tid, char *input, uint n) {
	int ret = 0;

	CHECK_INITED;

	LOCK_BIG_MUTEX;

	DIE_ON_INVAL_TID(tid);

	if(fwrite(input, 1, n, descs[tid].pty.master) < n)
		SYS_ERR("fwrite", input, after_lock);

after_lock:
	UNLOCK_BIG_MUTEX;
before_anything:
	return ret;
}

int DLLEXPORT ltm_simulate_output(int tid, char *input, uint n) {
	int ret = 0;

	CHECK_INITED;

	LOCK_BIG_MUTEX;

	DIE_ON_INVAL_TID(tid);

	if(fwrite(input, 1, n, descs[tid].pty.slave) < n)
		SYS_ERR("fwrite", input, after_lock);

after_lock:
	UNLOCK_BIG_MUTEX;
before_anything:
	return ret;
}

static int read_into_outputbuf(int tid) {
	uint buflen;
	int ret = 0;
	uchar *buf;

	if(ioctl(fileno(descs[tid].pty.master), FIONREAD, &buflen) == -1)
		SYS_ERR("ioctl", "FIONREAD", error);

	if(descs[tid].outputbuf) { /* add to the current buffer */
		buf = realloc(descs[tid].outputbuf, descs[tid].buflen + buflen);
		if(!buf) SYS_ERR("realloc", NULL, error);

		if(fread(buf + descs[tid].buflen, 1, buflen, descs[tid].pty.master) < buflen)
			SYS_ERR("fread", NULL, error);

		descs[tid].buflen += buflen;
	} else { /* create a new buffer */
		buf = malloc(buflen);
		if(!buf) SYS_ERR("malloc", NULL, error);

		if(fread(buf, 1, buflen, descs[tid].pty.master) < buflen)
			SYS_ERR("fread", NULL, error);

		descs[tid].buflen = buflen;
	}

	descs[tid].outputbuf = buf;

error:
	return ret;
}

int DLLEXPORT ltm_process_output(int tid) {
	int ret = 0;
	uchar *buf;
	uint i;

	CHECK_INITED;

#ifdef HAVE_LIBPTHREAD
	if(threading && !pthread_equal(pthread_self(), watchthread))
		LTM_ERR(ENOTSUP, "Threading is on, so you may not manually tell libterm to process the output", before_anything);
#endif

	LOCK_BIG_MUTEX;

	DIE_ON_INVAL_TID(tid);

	if(read_into_outputbuf(tid) == -1) PASS_ERR(after_lock);

	buf = descs[tid].outputbuf;

	for(i = 0; i < descs[tid].buflen; i++) {
		if(buf[i] > 0x7f) continue;

		if(buf[i] > '~' || buf[i] < ' ') {
			switch(buf[i]) {
				case '\n': /* line break */
					if(cursor_line_break(tid) == -1)
						PASS_ERR(after_lock);
					break;
				case '\r': /* carriage return */
					if(cursor_abs_move(tid, X, 0) == -1)
						PASS_ERR(after_lock);
					break;
				case '\b': /* backspace */
					if(cursor_rel_move(tid, LEFT, 1) == -1)
						PASS_ERR(after_lock);
					break;
				case '\v': /* vertical tab */
					if(cursor_vertical_tab(tid) == -1)
						PASS_ERR(after_lock);
					break;
				case '\t': /* horizontal tab */
					if(cursor_horiz_tab(tid) == -1)
						PASS_ERR(after_lock);
					break;
				case '\a': /* bell */
					cb_alert(tid);
					break;
			}

			continue;
		}

		if(descs[tid].cursor.x == descs[tid].cols-1 && descs[tid].curs_prev_not_set)
			if(cursor_wrap(tid) == -1)
				PASS_ERR(after_lock);

		if(descs[tid].set.ranges == NULL || descs[tid].set.nranges == 0 || memcmp(&TOPRANGE(&descs[tid].set)->end, &descs[tid].cursor, sizeof(struct point))) {
			if(range_add(&descs[tid].set) == -1) PASS_ERR(after_lock);

			TOPRANGE(&descs[tid].set)->action = ACT_COPY;
			TOPRANGE(&descs[tid].set)->val = 0;

			TOPRANGE(&descs[tid].set)->leftbound = 0;
			TOPRANGE(&descs[tid].set)->rightbound = descs[tid].cols-1;

			TOPRANGE(&descs[tid].set)->start.y = descs[tid].cursor.y;
			TOPRANGE(&descs[tid].set)->start.x = descs[tid].cursor.x;

			TOPRANGE(&descs[tid].set)->end.y = descs[tid].cursor.y;
			TOPRANGE(&descs[tid].set)->end.x = descs[tid].cursor.x;
		}

		descs[tid].screen[descs[tid].cursor.y][descs[tid].cursor.x] = buf[i];

		if(cursor_advance(tid) == -1)
			PASS_ERR(after_lock);

		TOPRANGE(&descs[tid].set)->end.x = descs[tid].cursor.x;
		TOPRANGE(&descs[tid].set)->end.y = descs[tid].cursor.y;
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
		if(!buf) SYS_ERR("malloc", NULL, after_lock);

		memcpy(buf, &descs[tid].outputbuf[i], descs[tid].buflen);

		free(descs[tid].outputbuf);
		descs[tid].outputbuf = buf;
	} else /* nothing done, exit */
		goto after_lock;

	if(descs[tid].lines_scrolled < descs[tid].lines) {
		/* this is the case where the original content has *not* been completely scrolled
		 * off the screen, and so it makes sense to tell the app to scroll the screen and
		 * only update things that have changed
		 */

		if(range_finish(&descs[tid].set) == -1) PASS_ERR(after_lock);

		cb_scroll_lines(tid);
		cb_update_ranges(tid, descs[tid].set.ranges);
	} else
		/* this is the case in which the entirety of the original content *has* been
		 * scrolled off the screen, so we might as well just reload the entire thing
		 */
		cb_refresh_screen(tid);

	descs[tid].lines_scrolled = 0;

	cb_refresh(tid);

	range_free(&descs[tid].set);

after_lock:
	UNLOCK_BIG_MUTEX;
before_anything:
	return ret;
}

#ifdef HAVE_LIBPTHREAD
/* use poll because there's no limit on the number of fds to watch */
void *watch_for_events() {
	int i, n, nfds, pollret, newtid;
	struct pollfd *fds = 0;
	char code, running = 1;
	void *ret = (void*)1;

	LOCK_BIG_MUTEX_THR;

	/* start out with just the pipe fd... */
	fds = malloc(sizeof(struct pollfd));
	if(!fds) SYS_ERR_THR("malloc", NULL, after_lock);

	fds[0].fd = fileno(pipefiles[0]);
	fds[0].events = POLLIN;

	nfds = 1;

	while(running) {
		UNLOCK_BIG_MUTEX_THR;

		pollret = poll(fds, nfds, -1);

		if(!pollret || (pollret == -1 && errno == EINTR)) continue;
		else if(pollret == -1) SYS_ERR_THR("poll", NULL, after_alloc);

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
						SYS_ERR_THR("fread", NULL, after_lock);

					if(code == EXIT_THREAD) {
						running = 0;
						break;
					}

					if(fread(&newtid, 1, sizeof(int), pipefiles[0]) < sizeof(int))
						SYS_ERR_THR("fread", NULL, after_lock);

					switch(code) {
						case NEW_TERM:
							fds = realloc(fds, (++nfds) * sizeof(struct pollfd));
							if(!fds) SYS_ERR_THR("realloc", NULL, after_lock);

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
									if(!fds) SYS_ERR_THR("realloc", NULL, after_lock);

									break;
								}

							cb_term_exit(newtid);

							break;
					}

				}
				else for(n = 0; n < next_tid; n++)
					if(descs[n].pid && fds[i].fd == fileno(descs[n].pty.master)) {
						if(ltm_process_output(n) == -1) PASS_ERR_PTR(after_lock);
						break;
					}

				fds[i].revents = 0;
			}
	}

after_lock:
	UNLOCK_BIG_MUTEX_THR;
after_alloc:
	if(fds) free(fds);
	if(!ret) cb_thread_died(thr_curerr);
	return ret;
}
#endif
