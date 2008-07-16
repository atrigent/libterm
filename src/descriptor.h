#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <sys/types.h>

#define DIE_ON_INVAL_TID(tid) \
	if(tid >= next_tid || descs[tid].allocated == 0) \
		LTM_ERR(EINVAL, "Invalid TID", after_lock);

#define DIE_ON_INVAL_TID_PTR(tid) \
	if(tid >= next_tid || descs[tid].allocated == 0) \
		LTM_ERR_PTR(EINVAL, "Invalid TID", after_lock);

/* some things won't work if MAINSCREEN isn't zero! don't
 * change this!
 */
#define MAINSCREEN 0
#define ALTSCREEN 1

#define RANGE_AREA 0
#define RANGE_RECT 1

struct point {
	ushort x;
	ushort y;
};

struct range {
	char type;

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

	struct point cursor;
	char curs_changed;
	char curs_prev_not_set;

	char cur_screen;
	uint **screen;
	uint **main_screen;
	uint **alt_screen;

	uchar *wrapped;

	uchar *outputbuf;
	uint buflen;

	struct range **ranges;
	uint nranges;

	uint lines_scrolled;
};

extern struct ltm_term_desc *descs;
extern int next_tid;

#endif
