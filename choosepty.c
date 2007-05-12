#include <stdlib.h>

#include "libterm.h"

/* in the future, the user will be able to specify the priority */
ptyfunc pty_func_prior[] = {
	alloc_unix98_pty,
	find_unused_bsd_pty,
	alloc_func_pty,
	NULL
};

struct ptydev * choose_pty_method() {
	struct ptydev * pty;
	int i, result;

	pty = malloc(sizeof(struct ptydev));
	pty->master = 0;

	for(i = 0; pty_func_prior[i]; i++) {
		result = (*pty_func_prior[i])(pty);

		if(result == 0 || result == -1) continue;
		
		break;
	}

	if(!pty->master) {
		free(pty);
		return NULL;
	}
	
	return pty;
}
