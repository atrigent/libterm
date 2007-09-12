#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <sys/types.h>

#define DIE_ON_INVAL_TID(tid) \
	if(tid >= next_tid || descs[tid].allocated == 0) \
		FIXABLE_LTM_ERR(EINVAL)

/* some things won't work if MAINSCREEN isn't zero! don't
 * change this!
 */
#define MAINSCREEN 0
#define ALTSCREEN 1

struct point {
	ushort x;
	ushort y;
};

struct area {
	struct point start;
	struct point end;
};

struct ltm_callbacks {
	int (*update_areas)(int, uint **, struct point *, struct area **);
	int (*refresh_screen)(int, uint **);
	int (*scroll_lines)(int, uint);

	/* many more in the future! */
};

struct ltm_term_desc {
	struct ptydev pty;

	struct ltm_callbacks cb;

	char allocated;
	char threading;

	char * shell;
	pid_t pid;

	ushort width;
	ushort height;

	struct point cursor;
	char curs_changed;

	char cur_screen;
	uint ** screen;
	uint ** main_screen;
	uint ** alt_screen;

	char * outputbuf;
	uint buflen;

	struct area ** areas;
	uint nareas;
};

extern struct ltm_term_desc * descs;
extern int next_tid;

#endif
