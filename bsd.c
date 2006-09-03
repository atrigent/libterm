#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "libterm.h"

static int next_bsd_pty(char * ident, int * init) {
	if(*init) {
		*init = 0;
		
		ident[0] = 'p';
		ident[1] = '0';
		
		return LTM_TRUE;
	}

	if(ident[1] == 'f') {
		if(ident[0] == 'z') ident[0] = 'a';
		else if(ident[0] == 'e') return LTM_FALSE;
		else ident[0]++;

		ident[1] = '0';
	}
	else if(ident[1] == '9') ident[1] = 'a';
	else ident[1]++;

	return LTM_TRUE;
}

/* FIXME: This needs to be edited when we have
 * a general infrastructure for trying different
 * PTY types.
 */
int find_unused_bsd_pty() {
	int init = 1;
	FILE *pty_file, *tty_file;
	char tty_path[11], pty_path[11], tty_spc[2];
	
	strcpy(tty_path, "/dev/tty");
	strcpy(pty_path, "/dev/pty");
	pty_path[10] = tty_path[10] = 0;

	while(next_bsd_pty(tty_spc, &init)) {
		pty_path[8] = tty_path[8] = tty_spc[0];
		pty_path[9] = tty_path[9] = tty_spc[1];

		pty_file = fopen(pty_path, "r+");
		if(!pty_file)
			printf("Could not open pty master %s: %s\n",
					pty_path, strerror(errno));
		else {
			tty_file = fopen(tty_path, "r+");
			if(!tty_file)
				printf("Could not open pty slave %s: %s\n",
						tty_path, strerror(errno));
			else {
				fclose(tty_file);
			}
			fclose(pty_file);
		}
	}

	return 0;
}
