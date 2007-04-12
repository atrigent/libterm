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

int alloc_func_pty(FILE ** omaster, FILE ** oslave) {
	FILE *master, *slave;
	int master_fd, slave_fd, err;
#if defined(HAVE_UNIX98_FUNCS)
	char * pts_name;
#endif

#if defined(HAVE_OPENPTY) || defined(HAVE_UNIX98_FUNCS)
# if defined(HAVE_UNIX98_FUNCS)
#  if defined(HAVE_POSIX_OPENPT)
	master_fd = posix_openpt(O_RDWR|O_NOCTTY);
	if(master_fd == -1) return 0;
#  elif defined(HAVE_GETPT)
	master_fd = getpt();
	if(master_fd == -1) return 0;
#  else
	master_fd = open("/dev/ptmx", O_RDWR|O_NOCTTY);
	if(master_fd == -1) return 0;
#  endif
	if(grantpt(master_fd) == -1) return 0;

	if(unlockpt(master_fd) == -1) return 0;

	pts_name = ptsname(master_fd);
	if(!pts_name) return 0;

	slave_fd = open(pts_name, O_RDWR|O_NOCTTY);
	if(slave_fd == -1) return 0;
# else
	if(openpty(&master_fd, &slave_fd, NULL, NULL, NULL) == -1) return 0;
# endif
	master = fdopen(master_fd, "r+");
	if(!master) return 0; /* don't really know what to do here, since
			       * the error could be "fatal" (i.e. shouldn't
			       * ever happen), but I also don't want to take
			       * away the opportunity to try other terminal
			       * types.
			       */

	slave = fdopen(slave_fd, "r+");
	if(!slave) return 0;

	*omaster = master;
	*oslave = slave;

	return 1;
#else
	return 0;
#endif
}
