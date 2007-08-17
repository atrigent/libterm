#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#define DIE_ON_INVAL_TID(tid) \
	if(tid >= next_desc || descriptors[tid].pty == NULL) \
		FIXABLE_LTM_ERR(EINVAL)

struct ltm_term_desc {
	struct ptydev * pty;

	uint width;
	uint height;

	char ** cur_screen;
	char ** main_screen;
};

extern struct ltm_term_desc * descriptors;
extern int next_desc;

#endif
