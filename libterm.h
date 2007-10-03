#ifndef LIBTERM_H
#define LIBTERM_H

#include <signal.h>
#include <stdio.h>

#ifdef WIN32
#  define extern extern __declspec(dllimport)
#endif

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

struct ltm_callbacks {
	int (*update_areas)(int, uint **, struct point *, struct area **);
	int (*refresh_screen)(int, uint **);
	int (*scroll_lines)(int, uint);
	int (*alert)(int);

	/* many more in the future! */
};

extern int ltm_term_alloc();
extern int ltm_term_init(int, FILE **);
extern int ltm_close(int);

extern int ltm_init();
extern int ltm_uninit();

extern int ltm_feed_input_to_program(int, char *, uint);
extern int ltm_set_window_dimensions(int, ushort, ushort);
extern int ltm_set_shell(int, char *);
extern int ltm_get_callbacks_ptr(int, struct ltm_callbacks **);
extern int ltm_toggle_threading(int);

extern int ltm_process_output(int);

extern int ltm_reload_sigchld_handler();
extern int ltm_set_sigchld_handler(struct sigaction * action);

extern int ltm_get_notifier(FILE **);

extern void ltm_set_error_dest(FILE *);

extern int ltm_is_line_wrapped(int, uint);

#ifdef WIN32
#  undef extern
#endif

#endif
