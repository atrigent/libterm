#ifndef SCREEN_H
#define SCREEN_H

#define SCR(tid, sid) (descs[tid].screens[sid])

#define SCR_FIELD(tid, field) SCR(tid, descs[tid].field)

#define CUR_SCR(tid) SCR_FIELD(tid, cur_screen)
#define CUR_INP_SCR(tid) SCR_FIELD(tid, cur_input_screen)

enum updateactions {
	UPD_CURS = 1,
	UPD_CURS_INVIS = 2,
	UPD_SCROLL = 4,
	UPD_GET_SET = 8
};

#define TRANSLATE_PT(pt, link) \
	do { \
		(pt).x += (link).origin.x; \
		(pt).y += (link).origin.y - (link).fromline; \
	} while(0)

struct link {
	struct point origin;

	ushort fromline;
	ushort toline;
};

struct cell {
	uint chr;
};

struct screen {
	char allocated;

	ushort lines;
	ushort cols;

	struct cell **matrix;
	uchar *wrapped;

	char curs_invisible;
	struct point cursor;
	char curs_prev_not_set;
	char autoscroll;

	struct list_node *downlinks;
	struct list_node *uplinks;
};

extern struct rangeset *record_update(int, int, enum updateactions);

extern int screen_set_dimensions(int, int, ushort, ushort);
extern int screen_alloc(int, char, ushort, ushort);

extern int screen_add_link(int, int, int, struct link *);
extern int screen_del_link(int, int, int);

extern void screen_set_autoscroll(int, int, char);
extern void screen_give_input_focus(int, int);
extern int screen_make_current(int, int);

extern int screen_inject_update(int, int, struct range *);

extern int screen_set_point(int, int, enum action, struct point *, uint);
extern int screen_clear_range(int, int, struct range *);
extern int screen_copy_range(int, int, int, struct range *, struct link *);

extern int screen_scroll_rect(int, int, struct range *);
extern int screen_scroll(int, int);

#endif
