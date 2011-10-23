#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libterm.h"

#if defined(HAVE_UNIX98_FUNCS)
# if defined(HAVE_POSIX_OPENPT) /* required for posix_openpt */
#  include <fcntl.h>
# elif !defined(HAVE_GETPT) /* required for open */
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <fcntl.h>
# endif
#elif defined(HAVE_OPENPTY)
# include <pty.h>
#endif

static int alloc_unix98_pty(struct ptydev *);
static int find_unused_bsd_pty(struct ptydev *);
static int alloc_func_pty(struct ptydev *);

/* in the future, the user will be able to specify the priority */
int (*pty_func_prior[])(struct ptydev *) = {
	alloc_unix98_pty,
	find_unused_bsd_pty,
	alloc_func_pty,
	NULL
};

int choose_pty_method(struct ptydev *pty) {
	int i, result, ret = 0;

	for(i = 0; pty_func_prior[i]; i++) {
		result = (*pty_func_prior[i])(pty);

		if(result == 0 || result == -1) continue;

		break;
	}

	if(!pty->master || !pty->slave) LTM_ERR(ENODEV, "Could not acquire a pseudoterminal pair", error);

	setbuf(pty->master, NULL);
	setbuf(pty->slave, NULL);

error:
	return ret;
}

static int alloc_unix98_pty(struct ptydev *pty) {
	FILE *master, *slave;
	char slave_path[32]; /* big enough */
	int unlock = 0, pty_num, ret = 1;

	master = fopen("/dev/ptmx", "r+");
	/* This should always be successful - if not, there's nothing anyone
	 * can do about it. */
	if(!master) SYS_ERR("fopen", "/dev/ptmx", error);

	/* all errors from ioctl are weird and unfixable */
	if(ioctl(fileno(master), TIOCSPTLCK, &unlock) == -1) SYS_ERR("ioctl", "TIOCSPTLCK", error);

	if(ioctl(fileno(master), TIOCGPTN, &pty_num) == -1) SYS_ERR("ioctl", "TIOCGPTN", error);

	sprintf(slave_path, "/dev/pts/%u", pty_num);

	slave = fopen(slave_path, "r+");
	if(!slave) SYS_ERR("fopen", slave_path, error);

	pty->type = UNIX98_PTY;
	pty->master = master;
	pty->slave = slave;

error:
	return ret;
}

static int next_bsd_pty(char *ident) {
	if(ident[1] == 'f') {
		if(ident[0] == 'z') ident[0] = 'a';
		else if(ident[0] == 'e') return 0;
		else ident[0]++;

		ident[1] = '0';
	}
	else if(ident[1] == '9') ident[1] = 'a';
	else ident[1]++;

	return 1;
}

static int find_unused_bsd_pty(struct ptydev *pty) {
	FILE *pty_file, *tty_file;
	char tty_path[11], pty_path[11], tty_spc[2] = "p0";
	int unlock = 0;

	strcpy(tty_path, "/dev/tty");
	strcpy(pty_path, "/dev/pty");
	pty_path[10] = tty_path[10] = 0;

	do {
		pty_path[8] = tty_path[8] = tty_spc[0];
		pty_path[9] = tty_path[9] = tty_spc[1];

		pty_file = fopen(pty_path, "r+");
		if(!pty_file) continue; /* open failed, go try next one */
		else {
			/* This doesn't appear to actually be necessary
			 * (according to the Linux kernel source), but let's do
			 * it anyway.
			 */
			if(ioctl(fileno(pty_file), TIOCSPTLCK, &unlock) == -1)
				continue;
			tty_file = fopen(tty_path, "r+");
			if(!tty_file) continue;

			pty->type = BSD_PTY;
			pty->master = pty_file;
			pty->slave = tty_file;

			return 1;
		}
	} while(next_bsd_pty(tty_spc));

	return 0;
}

static int alloc_func_pty(struct ptydev *pty) {
	FILE *master, *slave;
	int master_fd, slave_fd;
#if defined(HAVE_OPENPTY) || defined(HAVE_UNIX98_FUNCS)
#if defined(HAVE_UNIX98_FUNCS)
	char *pts_name;
#endif
	int ret = 1;
#endif

#if defined(HAVE_OPENPTY) || defined(HAVE_UNIX98_FUNCS)
# if defined(HAVE_UNIX98_FUNCS)
#  if defined(HAVE_POSIX_OPENPT)
	master_fd = posix_openpt(O_RDWR|O_NOCTTY);
	if(master_fd == -1) SYS_ERR("posix_openpt", NULL, error);
#  elif defined(HAVE_GETPT)
	master_fd = getpt();
	if(master_fd == -1) SYS_ERR("getpt", NULL, error);
#  else
	master_fd = open("/dev/ptmx", O_RDWR|O_NOCTTY);
	if(master_fd == -1) SYS_ERR("open", "/dev/ptmx", error);
#  endif
	if(grantpt(master_fd) == -1) SYS_ERR("grantpt", NULL, error);

	if(unlockpt(master_fd) == -1) SYS_ERR("unlockpt", NULL, error);

	pts_name = ptsname(master_fd);
	if(!pts_name) SYS_ERR("ptsname", NULL, error);

	slave_fd = open(pts_name, O_RDWR|O_NOCTTY);
	if(slave_fd == -1) SYS_ERR("open", pts_name, error);
# else
	if(openpty(&master_fd, &slave_fd, NULL, NULL, NULL) == -1) SYS_ERR("openpty", NULL, error);
# endif
	master = fdopen(master_fd, "r+");
	if(!master) SYS_ERR("fdopen", "fdopening master_fd", error);

	slave = fdopen(slave_fd, "r+");
	if(!slave) SYS_ERR("fdopen", "fdopening slave_fd", error);

	pty->type = FUNC_PTY;
	pty->master = master;
	pty->slave = slave;

error:
	return ret;
#else
	return 0;
#endif
}
