#ifndef ERROR_H
#define ERROR_H

#include <string.h>
#include <errno.h>

/* Error reporting policy in libterm:
 * If a system call that libterm calls returns an error, there are currently two
 * means in which these errors are communicated to the program using libterm:
 *
 * 1. Use LTM_ERR to set ltm_curerr.sys_func to NULL, set errno and
 *    ltm_curerr.err_no to the supplied value, set ltm_curerr.ltm_func to
 *    __func__, set the local variable ret to -1, and jump to the specified label
 *      This is used for errors which are generated by a libterm function in
 *      response to some value that was passed to it, usually by the application
 *      using libterm.
 * 2. Use SYS_ERR to set curer.sys_func to the supplied function name,
 *    set ltm_curerr.ltm_func to __func__, set err_no to errno and ret to -1,
 *    and jump to the specified label
 *      This is used for errors that originate from a system call that libterm
 *      makes, rather than from libterm itself.
 *
 * There are also variants of these macros ({LTM,SYS}_ERR_PTR) which set ret to
 * NULL instead of -1.
 *
 * Here's roughly what a libterm function call should look like (if it
 * returns errors):
 *
 * if(ltm_j_random_libterm_function(blorgh, zblat) == -1) {
 *      if(ltm_curerr.sys_func) {
 *           // system error, handle it accordingly
 *      }
 *      else {
 *           // libterm-generated error, handle it accordingly
 *      }
 * }
 *
 * It's entirely acceptible to simply do the same thing in both the system
 * error and libterm error case. If the distinction is useful to your
 * application, use it, or else you can ignore it.
 *
 * Luckily, you won't have to call very many libterm functions yourself,
 * so you don't have to partake in this elaborate ritual too often...
 */

/* This stuff is mostly obsolete, now that there is no concept of "fatal" and
 * "fixable" errors, only "libterm" and "system" ones. In addition, this mentions
 * trynode.c, which was removed. Keeping it here for hisorical reasons.
 *
 * Ok, I think it's high time I had a definitive policy on what errors use what
 * error handling method. Here's a list of the most common errors (note that
 * most of these are not really very common, but they are relative to the
 * REALLY obscure ones).
 *
 * This first group here I'm thinking will always use FATAL_ERR. This is because
 * they would either only be returned if there was an internal problem with
 * libterm or they are so broad that the problem would affect other programs
 * anyway and should be fixed. Detailed analysis:
 *
 * EFAULT - I don't know how this is used, since programs usually just segfault
 *          with a bad address, but whatever. If this happens, it's obviously
 *          because of an internal error or because the compiler is screwed up.
 * EINVAL - As far as I can tell, this would only happen if there's an internal
 *          problem. This usually means that one of the arguments to a function
 *          is invalid.
 * ETXTBSY- Why would libterm ever even try to open something with write access
 *          that could ever in a million years be an executable file? Something
 *          is wrong if we get this error.
 * ENOMEM - Program should close as soon as possible, I'm not going to try to
 *          write some sort of smart memory reclaiming mechanism.
 * EMFILE - libterm really shouldn't open that many things, unless you're
 *          managing a LOT of terminals with it... Also, libterm isn't
 *          responsible for the program using it. The user should raise their
 *          max open files per process setting.
 * ENFILE - libterm isn't responsible for the actions of the other programs
 *          on the system. The user should raise their max open files setting.
 * EPERM  - This should always be fatal. It means we can't do something that
 *          we're trying to do, and we'll never be able to do it (ex: making
 *          device nodes).
 *
 * I think these should definitely not be fatal:
 *
 * EACCES - VERY common error. This should definitely not be fatal.
 * ENOENT - Also a VERY common error.
 *
 * I think these ones should be fatal or not depending on the situation:
 *
 * EEXIST - A lot of the time, when we're trying to make a file and it already
 *          exists and is the right type, this is not a bad error at all. In
 *          other places, however, it can be bad. Thus, it depends.
 * ENOSPC - Very broad problem. However, in the case of a file buffer,
 *          libterm should probably not die. It should probably give a message
 *          to the user saying that the buffer will start being truncated at
 *          the top or is falling back to memory buffering.
 * EROFS  - VERY broad problem. In the case of a file buffer, again libterm
 *          should tell the user that buffering is not possible or is falling
 *          back to memory buffering.
 *
 * I'm not sure about these ones. To figure out if they should be fatal or not,
 * let's look at the sort of files libterm will likely be interacting with:
 *   pty master/slave files:
 *     /dev/ptmx
 *     /dev/tty[char][num]
 *     /dev/pty[char][num]
 *     used by trynode (see trynode.c):
 *       /dev/pty[num]
 *       /tmp/pts/[num]
 *       /tmp/pty[num]
 *       ~/.pts/[num]
 *       /tmp/pt/[num]
 *       ~/.pt/[num]
 *       more in the future?
 *   /var/log/{u,w}tmp? (recording logins)
 *   I'll probably use mkstemp or some such thing to make a file buffer
 *   The shell (ex: /bin/bash, /bin/sh, etc)
 *
 * As you can see, there isn't much to mess up here. Taking these names into
 * account, here is my verdict for the remaining errors:
 *
 * ELOOP  - This error happens when too many levels of symlinks have to be
 *          dereferenced. Is it very likely that this will happen, seeing as
 *          most of the paths we're using are very short? No, not really.
 *          I think this should be a fatal error.
 * ENOTDIR- This means that some component of the path is not a directory.
 *          Again, there isn't much chance of that happening with our limited
 *          set of paths. Fatal.
 * EISDIR - This means that the last component of the path is a directory and
 *          it shouldn't be. Again, it seems very unlikely that this will
 *          happen on a properly configured system. Fatal.
 */

/* taken from http://gcc.gnu.org/onlinedocs/gcc-4.1.1/gcc/Function-Names.html */
#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

#define DUMP_DEST (dump_dest ? dump_dest : stderr)
#define ERROR_DATA_LEN 256

#define ERR_MACRO_TMPL(sysfunc, err, _data, _ret, prefix, label) \
	do { \
		prefix##_curerr.sys_func = sysfunc; \
		prefix##_curerr.func = __func__; \
		prefix##_curerr.err_no = err; \
		strncpy(prefix##_curerr.data, _data ? _data : "", ERROR_DATA_LEN); \
		prefix##_curerr.file = __FILE__; \
		prefix##_curerr.line = __LINE__; \
		error_info_dump(prefix##_curerr); \
		errno = err; \
		ret = _ret; \
		goto label; \
	} while(0)

/* used for errors set by a libterm function
 */
#define LTM_ERR(err, data, label) \
	ERR_MACRO_TMPL(NULL, err, data, -1, ltm, label)

#define LTM_ERR_PTR(err, data, label) \
	ERR_MACRO_TMPL(NULL, err, data, NULL, ltm, label)

/* used for system call errors
 */
#define SYS_ERR(name, data, label) \
	ERR_MACRO_TMPL(name, errno, data, -1, ltm, label)

#define SYS_ERR_PTR(name, data, label) \
	ERR_MACRO_TMPL(name, errno, data, NULL, ltm, label)

/* these are only for use within watch_for_events()!!!
 */

#define LTM_ERR_THR(err, data, label) \
	ERR_MACRO_TMPL(NULL, err, data, NULL, thr, label)

#define SYS_ERR_THR(name, data, label) \
	ERR_MACRO_TMPL(name, errno, data, NULL, thr, label)

/* these are for passing errors on when an internal libterm
 * function returns with an error
 */

#define PASS_ERR(label) \
	do { \
		ret = -1; \
		goto label; \
	} while(0)

#define PASS_ERR_PTR(label) \
	do { \
		ret = NULL; \
		goto label; \
	} while(0)

#define ERROR_INFO_INITIALIZER {0, 0, 0, 0, "", 0}

struct error_info {
	const char *file;
	uint line;

	const char *func;
	char *sys_func;

	char data[ERROR_DATA_LEN];
	int err_no;
};

extern struct error_info ltm_curerr;
extern struct error_info thr_curerr;
extern FILE *dump_dest;

extern void error_info_dump(struct error_info);
extern void flush_repeated_errors();

#endif
