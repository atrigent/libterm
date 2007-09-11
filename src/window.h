#ifndef WINDOW_H
#define WINDOW_H

extern int ltm_set_window_dimensions(int, uint, uint);
extern int tcsetwinsz(int);
extern int scroll_screen(int, struct area **, uint *);

#endif
