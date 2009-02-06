#ifndef CURSOR_H
#define CURSOR_H

#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3

#define X 0
#define Y 1

extern int cursor_visibility(int, int, char);
extern int cursor_rel_move(int, int, char, ushort);
extern int cursor_abs_move(int, int, char, ushort);

extern int cursor_down(int, int);
extern int cursor_wrap(int, int);
extern int cursor_line_break(int, int);
extern int cursor_advance(int, int);
extern int cursor_vertical_tab(int, int);
extern int cursor_horiz_tab(int, int);

#endif
