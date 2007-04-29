#ifdef DESCRIPTOR_H
#define DESCRIPTOR_H

struct ltm_term_desc {
	struct ptydev * pty;
}

extern struct ltm_term_desc * descriptors;

#endif
