#ifndef LIBTERM_H
#define LIBTERM_H

#include <signal.h>
#include <stdio.h>

#ifdef WIN32
#  define extern extern __declspec(dllimport)
#endif

enum ltm_action {
	LTM_ACT_COPY,
	LTM_ACT_CLEAR,
	LTM_ACT_SCROLL
};

enum ltm_moduleclass {
	LTM_MOD_LIBTERM,
	LTM_MOD_CONF
};

#define LTM_ERROR_DATA_LEN 256

typedef unsigned int ltm_uint;
typedef unsigned short ltm_ushort;
typedef unsigned char ltm_uchar;

struct ltm_point {
	ltm_ushort x;
	ltm_ushort y;
};

struct ltm_range {
	enum ltm_action action;
	ltm_uint val;

	ltm_ushort leftbound;
	ltm_ushort rightbound;
	struct ltm_point start;
	struct ltm_point end;
};

struct ltm_error_info {
	const char *file;
	ltm_uint line;
	int mid;

	const char *func;
	char *sys_func;

	char data[LTM_ERROR_DATA_LEN];
	int err_no;
};

struct ltm_cell {
	ltm_uint chr;
};

struct ltm_callbacks {
	void (*update_ranges)(int, struct ltm_cell **, struct ltm_range **);
	void (*refresh)(int, char, struct ltm_point *);

	void (*scroll_lines)(int, ltm_uint);
	void (*refresh_screen)(int, struct ltm_cell **);
	void (*clear_screen)(int);

	void (*alert)(int);

	/* threading stuff */
	void (*thread_died)(struct ltm_error_info);
	void (*term_exit)(int);

	/* many more in the future! */
};

extern int ltm_term_alloc();
extern FILE *ltm_term_init(int);
extern int ltm_close(int);

extern int ltm_init();
extern int ltm_uninit();

extern int ltm_feed_input_to_program(int, char *, ltm_uint);
extern int ltm_simulate_output(int, char *, ltm_uint);
extern int ltm_set_window_dimensions(int, ltm_ushort, ltm_ushort);
extern int ltm_set_shell(int, char *);
extern int ltm_set_callbacks(struct ltm_callbacks *);
extern int ltm_set_threading(char);

extern int ltm_process_output(int);

extern int ltm_reload_sigchld_handler();
extern int ltm_set_sigchld_handler(struct sigaction *action);

extern FILE *ltm_get_notifier();

extern int ltm_set_error_dest(FILE *);

extern int ltm_set_config_entry(enum ltm_moduleclass, char *, char *, char *);

extern int ltm_is_line_wrapped(int, ltm_uint);

extern struct ltm_error_info ltm_curerr;

#ifdef WIN32
#  undef extern
#endif

#endif
