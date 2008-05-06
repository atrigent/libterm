#ifndef LIBTERM_H
#define LIBTERM_H

#include <signal.h>
#include <stdio.h>

#ifdef WIN32
#  define extern extern __declspec(dllimport)
#endif

#define ERROR_DATA_LEN 256

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

struct point {
	ushort x;
	ushort y;
};

struct area {
	struct point start;
	struct point end;
};

struct error_info {
	const char *file;
	uint line;

	const char * ltm_func;
	char * sys_func;

	char data[ERROR_DATA_LEN];
	int err_no;
};

struct ltm_callbacks {
	int (*update_areas)(int, uint **, struct point *, struct area **);
	int (*refresh_screen)(int, uint **);
	int (*scroll_lines)(int, uint);
	int (*alert)(int);

	/* threading stuff */
	int (*term_exit)(int);
	int (*thread_died)(struct error_info);

	/* many more in the future! */
};

extern int ltm_term_alloc();
extern FILE * ltm_term_init(int);
extern int ltm_close(int);

extern int ltm_init();
extern int ltm_uninit();

extern int ltm_feed_input_to_program(int, char *, uint);
extern int ltm_simulate_output(int, char *, uint);
extern int ltm_set_window_dimensions(int, ushort, ushort);
extern int ltm_set_shell(int, char *);
extern int ltm_set_callbacks(struct ltm_callbacks *);
extern int ltm_set_threading(char);

extern int ltm_process_output(int);

extern int ltm_reload_sigchld_handler();
extern int ltm_set_sigchld_handler(struct sigaction * action);

extern FILE * ltm_get_notifier();

extern int ltm_set_error_dest(FILE *);

extern int ltm_parse_config_text(char *);

extern int ltm_is_line_wrapped(int, uint);

extern struct error_info ltm_curerr;

#ifdef WIN32
#  undef extern
#endif

#endif
