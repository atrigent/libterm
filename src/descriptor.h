#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <sys/types.h>

#define DIE_ON_INVAL_TID(tid) \
	if((tid) >= next_tid || descs[tid].allocated == 0) \
		LTM_ERR(EINVAL, "Invalid TID", after_lock)

#define DIE_ON_INVAL_TID_PTR(tid) \
	if((tid) >= next_tid || descs[tid].allocated == 0) \
		LTM_ERR_PTR(EINVAL, "Invalid TID", after_lock)

#define ACT_COPY   0
#define ACT_CLEAR  1
#define ACT_SCROLL 2

struct point {
	ushort x;
	ushort y;
};

struct range {
	char action;
	uint val;

	ushort leftbound;
	ushort rightbound;
	struct point start;
	struct point end;
};

struct ltm_term_desc {
	char allocated;

	struct ptydev pty;

	char shell_disabled;
	char *shell;
	pid_t pid;

	ushort lines;
	ushort cols;

	int cur_screen;
	int cur_input_screen;
	struct screen *screens;
	int next_sid;

	uchar *outputbuf;
	uint buflen;

	struct rangeset set;

	uint lines_scrolled;

	char curs_changed;
};

extern struct ltm_term_desc *descs;
extern int next_tid;

#endif
