#ifndef PTYDEV_H
#define PTYDEV_H

#define UNIX98_PTY 0
#define BSD_PTY    1
#define FUNC_PTY   2

/* From here to... */
/* Since major is a function on SVR4, we can't use `ifndef major'.  */
#if MAJOR_IN_MKDEV
# include <sys/mkdev.h>
# define HAVE_MAJOR
#endif
#if MAJOR_IN_SYSMACROS
# include <sys/sysmacros.h>
# define HAVE_MAJOR
#endif
#ifdef major			/* Might be defined in sys/types.h.  */
# define HAVE_MAJOR
#endif

#ifndef HAVE_MAJOR
# define major(dev)  (((dev) >> 8) & 0xff)
# define minor(dev)  ((dev) & 0xff)
# define makedev(maj, min)  (((maj) << 8) | (min))
#endif
#undef HAVE_MAJOR

#if ! defined makedev && defined mkdev
# define makedev(maj, min)  mkdev (maj, min)
#endif
/* ...here is taken from the GNU Coreutils project. URL:
 * http://cvs.savannah.gnu.org/viewcvs/coreutils/src/system.h?rev=1.159&root=coreutils&view=markup
 * Lines 72 to 94 
 */

struct ptydev {
	char type;
	FILE * master;
	FILE * slave;
};

typedef int (*ptyfunc)(struct ptydev *);

extern int alloc_unix98_pty(struct ptydev *);
extern int find_unused_bsd_pty(struct ptydev *);
extern int alloc_func_pty(struct ptydev *);

extern struct ptydev * choose_pty_method();

#endif
