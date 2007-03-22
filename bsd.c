#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "libterm.h"

static int next_bsd_pty(char * ident) {
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
int find_unused_bsd_pty(FILE ** master, FILE ** slave) {
	FILE *pty_file, *tty_file;
	char tty_path[11], pty_path[11], tty_spc[2] = "p0";

	strcpy(tty_path, "/dev/tty");
	strcpy(pty_path, "/dev/pty");
	pty_path[10] = tty_path[10] = 0;

	do {
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

			*master = pty_file;
			*slave = tty_file;

			return 1;
		}
	} while(next_bsd_pty(tty_spc));

	return 0;
}
