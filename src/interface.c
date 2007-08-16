#include "libterm.h"

int ltm_feed_input_to_program(unsigned int tid, char * input, unsigned int n) {
	DIE_ON_INVAL_TID(tid)

	if(fwrite(input, 1, n, descriptors[tid].pty->master) < n)
		FATAL_ERR("fwrite", input)

	return 0;
}
