#ifndef SCREEN_H
#define SCREEN_H

#define SCR(tid, sid) (descs[tid].screens[sid])

#define CUR_SCR(tid) SCR(tid, descs[tid].cur_screen)
#define CUR_INP_SCR(tid) SCR(tid, descs[tid].cur_input_screen)

struct screen {
	char allocated;

	ushort lines;
	ushort cols;

	uint **matrix;
	uchar *wrapped;

	char curs_invisible;
	struct point cursor;
	char curs_prev_not_set;
	char autoscroll;
};

extern int screen_set_dimensions(int, int, ushort, ushort);
extern int screen_alloc(int, char, ushort, ushort);

extern void screen_set_autoscroll(int, int, char);
extern void screen_give_input_focus(int, int);
extern int screen_make_current(int, int);

extern int screen_set_point(int, int, char, struct point *, uint);

extern int screen_scroll(int, int);

#endif
