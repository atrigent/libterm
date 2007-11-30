#ifndef CALLBACKS_H
#define CALLBACKS_H

extern int check_callbacks(int);

extern int cb_update_areas(int);
extern int cb_refresh_screen(int);
extern int cb_scroll_lines(int, uint);
extern int cb_alert(int);
extern int cb_term_exit(int);

#endif
