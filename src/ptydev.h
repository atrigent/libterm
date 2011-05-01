#ifndef PTYDEV_H
#define PTYDEV_H

enum ptytype {
	UNIX98_PTY,
	BSD_PTY,
	FUNC_PTY
};

struct ptydev {
	enum ptytype type;
	FILE *master;
	FILE *slave;
};

extern int choose_pty_method(struct ptydev *);

#endif
