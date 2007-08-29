#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <sys/types.h>

#define DIE_ON_INVAL_TID(tid) \
	if(tid >= next_tid || descriptors[tid].allocated == 0) \
		FIXABLE_LTM_ERR(EINVAL)

struct ltm_term_desc {
	struct ptydev pty;

	char allocated;

	char * shell;

	pid_t pid;

	uint width;
	uint height;

	char ** cur_screen;
	char ** main_screen;
};

extern struct ltm_term_desc * descriptors;
extern int next_tid;

#endif
