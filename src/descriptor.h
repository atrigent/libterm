#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <sys/types.h>

#define DIE_ON_INVAL_TID(tid) \
	if(tid >= next_tid || descriptors[tid].allocated == 0) \
		FIXABLE_LTM_ERR(EINVAL)

/* some things won't work if MAINSCREEN isn't zero! don't
 * change this!
 */
#define MAINSCREEN 0
#define ALTSCREEN 1

struct ltm_term_desc {
	struct ptydev pty;

	char allocated;

	char * shell;

	pid_t pid;

	uint width;
	uint height;

	char cur_screen;
	char ** screen;
	char ** main_screen;
	char ** alt_screen;
};

extern struct ltm_term_desc * descriptors;
extern int next_tid;

#endif
