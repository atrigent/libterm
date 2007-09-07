#include <stdlib.h>
#include <stdio.h>

#include "libterm.h"

/* in the future, the user will be able to specify the priority */
ptyfunc pty_func_prior[] = {
	alloc_unix98_pty,
	find_unused_bsd_pty,
	alloc_func_pty,
	NULL
};

int choose_pty_method(struct ptydev * pty) {
	int i, result;

	for(i = 0; pty_func_prior[i]; i++) {
		result = (*pty_func_prior[i])(pty);

		if(result == 0 || result == -1) continue;
		
		break;
	}

	if(!pty->master || !pty->slave) FIXABLE_LTM_ERR(ENODEV)

	setbuf(pty->master, NULL);
	setbuf(pty->slave, NULL);
	
	return 0;
}
