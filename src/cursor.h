#ifndef CURSOR_H
#define CURSOR_H

#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 4

#define X 0
#define Y 1

extern void cursor_rel_move(int, char, uint);
extern void cursor_abs_move(int, char, uint);

extern void cursor_line_break(int);
extern void cursor_advance(int);
extern void cursor_carriage_return(int);

#endif
