#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "screen.h"

struct callbacks {
	void (*update_ranges)(int, struct cell **, struct range **);
	void (*refresh)(int, char, struct point *);

	void (*scroll_lines)(int, uint);
	void (*refresh_screen)(int, struct cell **);
	void (*clear_screen)(int, struct cell **);

	void (*alert)(int);

	/* threading stuff */
	void (*thread_died)(struct error_info);
	void (*term_exit)(int);

	/* many more in the future! */
};

extern int check_callbacks(struct callbacks *);

extern void cb_update_ranges(int, struct range **);
extern void cb_update_range(int, struct range *);
extern void cb_refresh(int);

extern void cb_update_screen(int, enum action, int);
extern void cb_scroll_lines(int);
extern void cb_refresh_screen(int);
extern void cb_clear_screen(int);

extern void cb_alert(int);

extern void cb_term_exit(int);
extern void cb_thread_died(struct error_info);

extern struct callbacks cbs;

#endif
