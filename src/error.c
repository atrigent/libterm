#include <string.h>

#include "libterm.h"

struct error_info DLLEXPORT ltm_curerr = ERROR_INFO_INITIALIZER;
struct error_info thr_curerr = ERROR_INFO_INITIALIZER;
FILE *dump_dest = 0;

static struct error_info last_err = ERROR_INFO_INITIALIZER;
static uint repeats = 0;

void flush_repeated_errors() {
	if(repeats) {
		fprintf(DUMP_DEST, "Previous libterm error repeats %u times\n", repeats);
		memset(&last_err, 0, sizeof(struct error_info));
		repeats = 0;
	}
}

void error_info_dump(struct error_info err) {
	FILE *err_dest = DUMP_DEST;

	if(!memcmp(&last_err, &err, sizeof(struct error_info))) {
		repeats++;
		return;
	} else flush_repeated_errors();

	fprintf(err_dest, "libterm ");

	if(err.sys_func) fprintf(err_dest, "system ");

	fprintf(err_dest, "error:\n");

	fprintf(err_dest, "\tFile and line #: %s:%u\n", err.file, err.line);

	fprintf(err_dest, "\tOriginating function: %s\n", err.func);

	if(err.sys_func)
		fprintf(err_dest, "\tOriginating system function: %s\n", err.sys_func);

	fprintf(err_dest, "\tError: %s (%i numerical)\n", strerror(err.err_no), err.err_no);

	/* FIXME: this should use some sort of hex dumping function */
	if(err.data[0]) fprintf(err_dest, "\tData: %.*s\n", ERROR_DATA_LEN, err.data);

	memcpy(&last_err, &err, sizeof(struct error_info));
}

int DLLEXPORT ltm_set_error_dest(FILE *dest) {
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
