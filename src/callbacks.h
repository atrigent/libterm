#ifndef CALLBACKS_H
#define CALLBACKS_H

struct ltm_callbacks {
	int (*update_areas)(int, uint **, struct point *, struct area **);
	int (*refresh_screen)(int, uint **);
	int (*scroll_lines)(int, uint);
	int (*alert)(int);
	int (*term_exit)(int);

	/* many more in the future! */
};

extern int check_callbacks(struct ltm_callbacks *);

extern int cb_update_areas(int);
extern int cb_refresh_screen(int);
extern int cb_scroll_lines(int, uint);
extern int cb_alert(int);
extern int cb_term_exit(int);

extern struct ltm_callbacks cbs;

#endif
