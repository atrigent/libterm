#ifndef CURSOR_H
#define CURSOR_H

enum direction {
	UP,
	DOWN,
	LEFT,
	RIGHT
};

enum axis {
	X,
	Y
};

extern int cursor_visibility(int, int, char);
extern int cursor_rel_move(int, int, enum direction, ushort);
extern int cursor_abs_move(int, int, enum axis, ushort);

extern int cursor_down(int, int);
extern int cursor_wrap(int, int);
extern int cursor_line_break(int, int);
extern int cursor_advance(int, int);
extern int cursor_vertical_tab(int, int);
extern int cursor_horiz_tab(int, int);

#endif
