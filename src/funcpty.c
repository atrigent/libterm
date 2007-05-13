#include <stdio.h>

/* need to change these once I have a configure script... */

#if defined(HAVE_UNIX98_FUNCS)
# if defined(HAVE_POSIX_OPENPT)
#  define _XOPEN_SOURCE 600
#  include <fcntl.h>
# elif !defined(HAVE_GETPT)
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <fcntl.h>
# endif
# if !defined(HAVE_POSIX_OPENPTY)
#  define _XOPEN_SOURCE
# endif
# include <stdlib.h>
#elif defined(HAVE_OPENPTY)
# include <pty.h>
#endif

int alloc_func_pty(struct ptydev * pty) {
	FILE *master, *slave;
	int master_fd, slave_fd, err;
#if defined(HAVE_UNIX98_FUNCS)
	char * pts_name;
#endif

#if defined(HAVE_OPENPTY) || defined(HAVE_UNIX98_FUNCS)
# if defined(HAVE_UNIX98_FUNCS)
#  if defined(HAVE_POSIX_OPENPT)
	master_fd = posix_openpt(O_RDWR|O_NOCTTY);
	if(master_fd == -1) FATAL_ERR("posix_openpt", NULL)
#  elif defined(HAVE_GETPT)
	master_fd = getpt();
	if(master_fd == -1) FATAL_ERR("getpt", NULL)
#  else
	master_fd = open("/dev/ptmx", O_RDWR|O_NOCTTY);
	if(master_fd == -1) FATAL_ERR("open", "/dev/ptmx")
#  endif
	if(grantpt(master_fd) == -1) FATAL_ERR("grantpt", NULL)

	if(unlockpt(master_fd) == -1) FATAL_ERR("unlockpt", NULL)

	pts_name = ptsname(master_fd);
	if(!pts_name) FATAL_ERR("ptsname", NULL)

	slave_fd = open(pts_name, O_RDWR|O_NOCTTY);
	if(slave_fd == -1) FATAL_ERR("open", pts_name)
# else
	if(openpty(&master_fd, &slave_fd, NULL, NULL, NULL) == -1) FATAL_ERR("openpty", NULL)
# endif
	master = fdopen(master_fd, "r+");
	if(!master) FATAL_ERR("fdopen", "fdopening master_fd")

	slave = fdopen(slave_fd, "r+");
	if(!slave) FATAL_ERR("fdopen", "fdopening slave_fd")

	pty->type = FUNC_PTY;
	pty->master = master;
	pty->slave = slave;

	return 1;
#else
	return 0;
#endif
}
