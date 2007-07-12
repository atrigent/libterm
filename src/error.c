#include <string.h>
#include <stdio.h>

#include "libterm.h"

struct error_info curerr = {0, 0, 0};
int always_dump = 0;
FILE * dump_dest = stderr;

void error_info_dump(struct error_info err, char * data, int recover) {
	char * err_str;

	fprintf(dump_dest, "libterm ");

	if(!recover) fprintf(dump_dest, "un");
	fprintf(dump_dest, "recoverable ");

	if(!err.sys_func) fprintf(dump_dest, "non-system ");

	fprintf(dump_dest, "error:\n");

	fprintf(dump_dest, "\tOriginating libterm function: %s\n", err.ltm_func);

	if(err.sys_func)
		fprintf(dump_dest, "\tOriginating system function: %s\n", err.sys_func);

	err_str = strerror(err.err_no);
	fprintf(dump_dest, "\tError: %s (%i numerical)\n", err_str, err.err_no);

	/* FIXME: this should use some sort of hex dumping function */
	if(data) fprintf(dump_dest, "\tData: %s\n", data);
}