#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <poll.h>

#include "libterm.h"
#include "linkedlist.h"
#include "callbacks.h"
#include "cursor.h"
#include "screen.h"
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

	if(ioctl(fileno(descs[tid].pty.master), FIONREAD, &buflen) == -1)
		SYS_ERR("ioctl", "FIONREAD", error);

	if(descs[tid].outputbuf) { /* add to the current buffer */
		descs[tid].outputbuf = realloc(descs[tid].outputbuf, descs[tid].buflen + buflen);
		if(!descs[tid].outputbuf) SYS_ERR("realloc", NULL, error);

		if(fread(descs[tid].outputbuf + descs[tid].buflen, 1, buflen, descs[tid].pty.master) < buflen)
			SYS_ERR("fread", NULL, error);

		descs[tid].buflen += buflen;
	} else { /* create a new buffer */
		descs[tid].outputbuf = malloc(buflen);
		if(!descs[tid].outputbuf) SYS_ERR("malloc", NULL, error);

		if(fread(descs[tid].outputbuf, 1, buflen, descs[tid].pty.master) < buflen)
			SYS_ERR("fread", NULL, error);

		descs[tid].buflen = buflen;
	}

error:
	return ret;
}

/* This function essentially does three things:
 * 1. Translates the lines_scrolled number into an ACT_SCROLL update
 * 2. Computes what the "copy region" should be (this also has to do with scrolling, see below)
 * 3. Figures out which updates are within the linked area and either ignores or
 *    restricts them accordingly
 *
 * Note that this function does NOT change any coordinates so that they refer to where
 * the linked region appears on the destination screen. It does do most of the work to
 * make this possible, however.
 */
static int translate_update(int tid, struct update *up, struct link *curlink, struct rangeset *newset) {
	ushort copy_fromline, copy_toline;
	struct range *currange;
	char need_copy = 0;
	uint i;

	if(up->lines_scrolled) {
		if(range_add(newset) == -1) return -1;

		TOPRANGE(newset)->action = ACT_SCROLL;
		TOPRANGE(newset)->val = up->lines_scrolled;

		TOPRANGE(newset)->leftbound = TOPRANGE(newset)->start.x = 0;
		TOPRANGE(newset)->start.y = curlink->fromline;
		TOPRANGE(newset)->rightbound = TOPRANGE(newset)->end.x = SCR(tid, up->sid).cols-1;
		TOPRANGE(newset)->end.y = curlink->toline;

		/* The copy region is best described with a diagram:
		 *
		 * +---------------------------+
		 * |                           |
		 * |                           |
		 * |                           | <- fromline
		 * |                           |
		 * |                           |
		 * |                           |
		 * |                           |
		 * |                           |
		 * |                           |
		 * |                           | <- toline
		 * |                           | \
		 * |                           | |  copy region
		 * |                           | /
		 * +---------------------------+
		 *
		 * This diagram assumes we're scrolling up.
		 *
		 * When we scroll and the toline is not at the bottom of
		 * the screen, the region which starts out where the
		 * "copy region" is marked in the above diagram is going
		 * to need special handling. Since it was not already
		 * in the linked region, it will not magically appear
		 * when scrolling happens. In addition, it will not be
		 * part of the propagated updates because it already
		 * existed - it was already written by previous updates.
		 * Therefore, we need to copy it. The calculations
		 * below are for figuring out where the copy region
		 * is after scrolling.
		 */
		if(
			curlink->toline != SCR(tid, up->sid).lines-1 &&
			curlink->fromline + up->lines_scrolled <= SCR(tid, up->sid).lines-1UL
		) {
			copy_fromline = curlink->fromline + up->lines_scrolled < curlink->toline+1UL ?
				curlink->toline+1 - up->lines_scrolled :
				curlink->fromline;

			copy_toline = curlink->toline + up->lines_scrolled > SCR(tid, up->sid).lines-1UL ?
				SCR(tid, up->sid).lines-1 - up->lines_scrolled :
				curlink->toline;

			need_copy = 1;
		}

		if(need_copy) {
			if(range_add(newset) == -1) return -1;

			TOPRANGE(newset)->action = ACT_COPY;
			TOPRANGE(newset)->val = 0;

			TOPRANGE(newset)->leftbound = TOPRANGE(newset)->start.x = 0;
			TOPRANGE(newset)->start.y = copy_fromline;
			TOPRANGE(newset)->rightbound = TOPRANGE(newset)->end.x = SCR(tid, up->sid).cols-1;
			TOPRANGE(newset)->end.y = copy_toline;
		}
	}

	for(i = 0; i < up->set.nranges; i++) {
		currange = up->set.ranges[i];

		/* if the current range is not in the current link at all, try the next link */
		if(currange->end.y < curlink->fromline || currange->start.y > curlink->toline) continue;

		/* if the current range is completely within the copy region, ignore it */
		if(need_copy &&
			(currange->start.y >= curlink->fromline ? currange->start.y : curlink->fromline) >= copy_fromline &&
			(currange->end.y <= curlink->toline ? currange->end.y : curlink->toline) <= copy_toline
		) continue;

		if(range_add(newset) == -1) return -1;

		TOPRANGE(newset)->action = currange->action;
		TOPRANGE(newset)->val = currange->val;

		TOPRANGE(newset)->leftbound = currange->leftbound;
		TOPRANGE(newset)->rightbound = currange->rightbound;

		if(currange->start.y >= curlink->fromline) {
			/* if the start of the range is inside the link, the restricted range will
			 * start where the original range starts
			 */
			TOPRANGE(newset)->start.y = currange->start.y;
			TOPRANGE(newset)->start.x = currange->start.x;
		} else {
			/* if the start of the range is before the link, the restricted range will
			 * start where the link starts
			 */
			TOPRANGE(newset)->start.y = curlink->fromline;
			TOPRANGE(newset)->start.x = currange->leftbound;
		}

		if(currange->end.y <= curlink->toline) {
			/* if the end of the range is inside the link, the restricted range will
			 * end where the original range ends
			 */
			TOPRANGE(newset)->end.y = currange->end.y;
			TOPRANGE(newset)->end.x = currange->end.x;
		} else {
			/* if the original range ends after the current link, the restricted
			 * range will end where the link ends
			 */
			TOPRANGE(newset)->end.y = curlink->toline;
			TOPRANGE(newset)->end.x = currange->rightbound;
		}

		if(need_copy) {
			/* if the current range overlaps the copy region, we need to restrict it
			 * further or split it up (depending on how it overlaps) in order to avoid
			 * telling the program using libterm to update the same area twice
			 */

			if(TOPRANGE(newset)->start.y >= copy_fromline && TOPRANGE(newset)->start.y <= copy_toline) {
				TOPRANGE(newset)->start.y = copy_toline+1;
				TOPRANGE(newset)->start.x = TOPRANGE(newset)->leftbound;
			} else if(TOPRANGE(newset)->end.y >= copy_fromline && TOPRANGE(newset)->end.y <= copy_toline) {
				TOPRANGE(newset)->end.y = copy_fromline-1;
				TOPRANGE(newset)->end.x = TOPRANGE(newset)->rightbound;
			} else if(TOPRANGE(newset)->start.y < copy_fromline && TOPRANGE(newset)->end.y > copy_toline) {
				/* the copy region is in the middle of the current range, so we need to split
				 * the current range up
				 */

				if(range_add(newset) == -1) return -1;

				memcpy(TOPRANGE(newset), NRANGE(newset, 2), sizeof(struct range));

				TOPRANGE(newset)->start.x = TOPRANGE(newset)->leftbound;
				TOPRANGE(newset)->start.y = copy_toline+1;

				NRANGE(newset, 2)->end.x = NRANGE(newset, 2)->rightbound;
				NRANGE(newset, 2)->end.y = copy_fromline-1;
			}
		}
	}

	return 0;
}

int propagate_updates(int tid, struct update *up) {
	struct list_node *curnode;
	struct update newup;
	struct point newcurs;
	struct link *curlink;
	int tosid, ret = 0;
	uint i;

	newup.set.ranges = NULL;
	newup.set.nranges = 0;

	for(curnode = SCR(tid, up->sid).uplinks; curnode; curnode = curnode->next) {
		tosid = curnode->u.intkey;
		curlink = curnode->data;

		if(translate_update(tid, up, curlink, &newup.set) == -1)
			PASS_ERR(after_newup_set_alloc);

		if(tosid == descs[tid].cur_screen && newup.set.nranges) {
			descs[tid].win_ups = realloc(descs[tid].win_ups, ++descs[tid].win_nups * sizeof(struct rangeset));
			if(!descs[tid].win_ups) SYS_ERR("realloc", NULL, after_newup_set_alloc);

			memset(&descs[tid].win_ups[descs[tid].win_nups-1], 0, sizeof(struct rangeset));
		}

		for(i = 0; i < newup.set.nranges; i++) {
			if(newup.set.ranges[i]->action == ACT_COPY)
				if(screen_copy_range(tid, up->sid, tosid, newup.set.ranges[i], curlink) == -1)
					PASS_ERR(after_newup_set_alloc);

			newup.set.ranges[i]->leftbound += curlink->origin.x;
			newup.set.ranges[i]->rightbound += curlink->origin.x;
			TRANSLATE_PT(newup.set.ranges[i]->start, *curlink);
			TRANSLATE_PT(newup.set.ranges[i]->end, *curlink);

			if(newup.set.ranges[i]->action == ACT_SCROLL) {
				if(screen_scroll_rect(tid, tosid, newup.set.ranges[i]) == -1)
					PASS_ERR(after_newup_set_alloc);
			} else if(newup.set.ranges[i]->action == ACT_CLEAR) {
				if(screen_clear_range(tid, tosid, newup.set.ranges[i]) == -1)
					PASS_ERR(after_newup_set_alloc);
			}
		}

		if(up->curs_changed) {
			newcurs.y = SCR(tid, up->sid).cursor.y;
			newcurs.x = SCR(tid, up->sid).cursor.x;

			if(newcurs.y < curlink->fromline || newcurs.y > curlink->toline || SCR(tid, up->sid).curs_invisible) {
				if(cursor_visibility(tid, tosid, 0) == -1) PASS_ERR(after_newup_set_alloc);
			} else {
				if(cursor_visibility(tid, tosid, 1) == -1) PASS_ERR(after_newup_set_alloc);

				TRANSLATE_PT(newcurs, *curlink);
				if(cursor_abs_move(tid, tosid, X, newcurs.x) == -1) PASS_ERR(after_newup_set_alloc);
				if(cursor_abs_move(tid, tosid, Y, newcurs.y) == -1) PASS_ERR(after_newup_set_alloc);
			}
		}

		newup.sid = tosid;
		newup.curs_changed = up->curs_changed;
		newup.lines_scrolled = 0;

		if(propagate_updates(tid, &newup) == -1)
			PASS_ERR(after_newup_set_alloc);

after_newup_set_alloc:
		range_free(&newup.set);

		if(ret == -1) break;
	}

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

	descs[tid].old_cur_screen = descs[tid].cur_screen;

	for(i = 0; i < descs[tid].buflen; i++) {
		if(buf[i] > 0x7f) continue;

		if(buf[i] > '~' || buf[i] < ' ') {
			switch(buf[i]) {
				case '\n': /* line break */
					if(cursor_line_break(tid, descs[tid].cur_input_screen) == -1)
						PASS_ERR(after_lock);
					break;
				case '\r': /* carriage return */
					if(cursor_abs_move(tid, descs[tid].cur_input_screen, X, 0) == -1)
						PASS_ERR(after_lock);
					break;
				case '\b': /* backspace */
					if(cursor_rel_move(tid, descs[tid].cur_input_screen, LEFT, 1) == -1)
						PASS_ERR(after_lock);
					break;
				case '\v': /* vertical tab */
					if(cursor_vertical_tab(tid, descs[tid].cur_input_screen) == -1)
						PASS_ERR(after_lock);
					break;
				case '\t': /* horizontal tab */
					if(cursor_horiz_tab(tid, descs[tid].cur_input_screen) == -1)
						PASS_ERR(after_lock);
					break;
				case '\a': /* bell */
					cb_alert(tid);
					break;
			}

			continue;
		}

		if(CUR_INP_SCR(tid).cursor.x == CUR_INP_SCR(tid).cols-1 && CUR_INP_SCR(tid).curs_prev_not_set)
			if(cursor_wrap(tid, descs[tid].cur_input_screen) == -1)
				PASS_ERR(after_lock);

		if(screen_set_point(tid, descs[tid].cur_input_screen, ACT_COPY, &CUR_INP_SCR(tid).cursor, buf[i]) == -1)
			PASS_ERR(after_lock);

		if(cursor_advance(tid, descs[tid].cur_input_screen) == -1)
			PASS_ERR(after_lock);
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

	for(i = 0; i < descs[tid].scr_nups; i++)
		if(propagate_updates(tid, &descs[tid].scr_ups[i]) == -1)
			PASS_ERR(after_lock);

	if(descs[tid].old_cur_screen == descs[tid].cur_screen) {
		/* This is the case in which the screen that was the currently active screen
		 * at the end of the last output processing session is still the active screen,
		 * so we can use the updates that we have stored to only ask the program using
		 * libterm to update stuff that has changed.
		 */

		for(i = 0; i < descs[tid].win_nups; i++) {
			if(descs[tid].win_ups[i].ranges[0]->val < CUR_INP_SCR(tid).lines) {
				/* this is the case where the original content of the link has *not* been completely scrolled
				 * off the screen, and so it makes sense to tell the app to scroll the link and
				 * only update things that have changed
				 */

				if(range_finish(&descs[tid].win_ups[i]) == -1)
						PASS_ERR(after_lock);

				cb_update_ranges(tid, descs[tid].win_ups[i].ranges);
			} else {
				/* this is the case in which the entirety of the original content of the link *has* been
				 * scrolled off the screen, so we might as well just reload the entire thing
				 */

				cb_update_range(tid, descs[tid].win_ups[i].ranges[0]);
			}
		}

		if(descs[tid].lines_scrolled >= CUR_SCR(tid).lines) cb_clear_screen(tid);
		else if(descs[tid].lines_scrolled) cb_scroll_lines(tid);

		if(descs[tid].set.nranges) {
			if(range_finish(&descs[tid].set) == -1) PASS_ERR(after_lock);
			cb_update_ranges(tid, descs[tid].set.ranges);
		}
	} else {
		/* This is the case in which the currently active screen has changed since
		 * the last output processing session, so all we can do is to tell the app
		 * using libterm to completely reload the screen.
		 */
		cb_refresh_screen(tid);
	}

	descs[tid].lines_scrolled = 0;

	cb_refresh(tid);

	for(i = 0; i < descs[tid].scr_nups; i++)
		range_free(&descs[tid].scr_ups[i].set);

	free(descs[tid].scr_ups);
	descs[tid].scr_ups = NULL;
	descs[tid].scr_nups = 0;

	for(i = 0; i < descs[tid].win_nups; i++)
		range_free(&descs[tid].win_ups[i]);

	free(descs[tid].win_ups);
	descs[tid].win_ups = NULL;
	descs[tid].win_nups = 0;

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
	enum threadaction code;
	struct pollfd *fds = 0;
	void *ret = (void*)1;
	char running = 1;

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
					if(fread(&code, 1, sizeof(enum threadaction), pipefiles[0]) < sizeof(enum threadaction))
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
						case EXIT_THREAD: /* make gcc happy */
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
