#include <string.h>

#include "libterm.h"

struct error_info DLLEXPORT ltm_curerr = {0, 0, 0, 0, 0};
struct error_info thr_curerr = {0, 0, 0, 0, 0};
FILE * dump_dest = 0;

void error_info_dump(struct error_info err, char * data) {
	char * err_str;

	fprintf(dump_dest, "libterm ");

	if(!err.sys_func) fprintf(dump_dest, "non-system ");

	fprintf(dump_dest, "error:\n");

	fprintf(dump_dest, "\tFile and line #: %s:%u\n", err.file, err.line);

	fprintf(dump_dest, "\tOriginating libterm function: %s\n", err.ltm_func);

	if(err.sys_func)
		fprintf(dump_dest, "\tOriginating system function: %s\n", err.sys_func);

	err_str = strerror(err.err_no);
	fprintf(dump_dest, "\tError: %s (%i numerical)\n", err_str, err.err_no);

	/* FIXME: this should use some sort of hex dumping function */
	if(data) fprintf(dump_dest, "\tData: %s\n", data);
}
