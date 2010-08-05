#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <sys/types.h>

#include "ptydev.h"

#define DIE_ON_INVAL_TID(tid) \
	if((tid) >= next_tid || descs[tid].allocated == 0) \
		LTM_ERR(EINVAL, "Invalid TID", after_lock)

#define DIE_ON_INVAL_TID_PTR(tid) \
	if((tid) >= next_tid || descs[tid].allocated == 0) \
		LTM_ERR_PTR(EINVAL, "Invalid TID", after_lock)

enum action {
	ACT_COPY,
	ACT_CLEAR,
	ACT_SCROLL
};

struct point {
	ushort x;
	ushort y;
};

struct range {
	enum action action;
	uint val;

	ushort leftbound;
	ushort rightbound;
	struct point start;
	struct point end;
};

struct term_desc {
	char allocated;

	struct ptydev pty;

	char shell_disabled;
	char *shell;
	pid_t pid;

	ushort lines;
	ushort cols;

	int old_cur_screen;
	int cur_screen;
	int cur_input_screen;
	struct screen *screens;
	int next_sid;

	uchar *outputbuf;
	uint buflen;

	/* updates that happened to different parts
	 * of the window screen during update propagation
	 * and updates that were written directly to the
	 * window screen
	 */
	struct rangeset set;

	uint lines_scrolled;

	/* whether the cursor changed, because there's only one cursor
	 * on the window screen
	 */
	char curs_changed;
};

extern struct term_desc *descs;
extern int next_tid;

#endif
