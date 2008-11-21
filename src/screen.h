#ifndef SCREEN_H
#define SCREEN_H

#define SCR(tid, sid) (descs[tid].screens[sid])

#define CUR_SCR(tid) SCR(tid, descs[tid].cur_screen)
#define CUR_INP_SCR(tid) SCR(tid, descs[tid].cur_input_screen)

#define UPD_CURS       1
#define UPD_CURS_INVIS 2
#define UPD_SCROLL     4
#define UPD_GET_SET    8

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

extern struct rangeset *record_update(int, int, char);

extern int screen_set_dimensions(int, int, ushort, ushort);
extern int screen_alloc(int, char, ushort, ushort);

extern void screen_set_autoscroll(int, int, char);
extern void screen_give_input_focus(int, int);
extern int screen_make_current(int, int);

extern int screen_set_point(int, int, char, struct point *, uint);

extern int screen_scroll(int, int);

#endif
