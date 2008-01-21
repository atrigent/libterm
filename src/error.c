#include <string.h>

#include "libterm.h"

struct error_info DLLEXPORT ltm_curerr = {0, 0, 0, 0, 0};
struct error_info thr_curerr = {0, 0, 0, 0, 0};
FILE * dump_dest = 0;

void error_info_dump(struct error_info err, char * data) {
	FILE *err_dest = dump_dest ? dump_dest : stderr;
	char *err_str;

	fprintf(err_dest, "libterm ");

	if(!err.sys_func) fprintf(err_dest, "non-system ");

	fprintf(err_dest, "error:\n");

	fprintf(err_dest, "\tFile and line #: %s:%u\n", err.file, err.line);

	fprintf(err_dest, "\tOriginating libterm function: %s\n", err.ltm_func);

	if(err.sys_func)
		fprintf(err_dest, "\tOriginating system function: %s\n", err.sys_func);

	err_str = strerror(err.err_no);
	fprintf(err_dest, "\tError: %s (%i numerical)\n", err_str, err.err_no);

	/* FIXME: this should use some sort of hex dumping function */
	if(data) fprintf(err_dest, "\tData: %s\n", data);
}

int DLLEXPORT ltm_set_error_dest(FILE * dest) {
	int ret = 0;

	if(init_state == INIT_DONE) {
		LOCK_BIG_MUTEX;
	}

	dump_dest = dest;

	if(init_state == INIT_DONE) {
		UNLOCK_BIG_MUTEX;
	}

before_anything:
	return ret;
}
