#ifndef ERROR_H
#define ERROR_H

#include <errno.h>

/* Error reporting policy in libterm:
 * If a system call that libterm calls returns an error, there are currently three
 * means in which these errors are communicated to the program using libterm:
 *
 * 1. Use FIXABLE_SYS_ERR to set curerr, leave errno alone, and return -1
 *      This is used when an error is returned by a system call which libterm
 *      uses, but the application using libterm could be at fault. This way it
 *      seems as if the error was actually generated by the libterm function,
 *      not the underlying system call. In this case, the documentation for the
 *      libterm function will describe the meaning of each errno value. In
 *      addition, curerr will be set appropriately for more detailed information
 *      on where the error occured.
 * 2. Use FIXABLE_LTM_ERR to set curerr.sys_func to NULL, set errno and
 *    curerr.err_no, set curerr.ltm_func to __func__, and return -1
 *      This is used for errors which really originate from a libterm function
 *      instead of a system call which a libterm function calls. This would look
 *      exactly the same as error method #1 except that curerr.sys_func is set
 *      to NULL because there's really no more information to be given.
 * 3. Use FATAL_ERR to set curerr, set errno to 0, and return -1
 *      This would be mainly for weird errors not seen very often. The
 *      application should not try to fix errors reported using this method. The
 *      libterm function returns -1 and sets errno to zero so that the
 *      application would be able to tell this type of error apart from a
 *      normal error.
 *
 *      Here's roughly what a libterm function call should look like (if it
 *      returns errors):
 *
 *      if(ltm_j_random_libterm_function(blorgh, zblat) == -1) {
 *           if(!errno) {
 *                // oops! fatal error! red alert! close program ASAP!!!
 *           }
 *           else if(!curerr.sys_func) {
 *                // fixable libterm error. use yer own error reporting
 *                // thingee or try to fix it!!!
 *           }
 *           else {
 *                // fixable system error. usually the only option here
 *                // is to use your own error reporting mechanism, because
 *                // there are too damn many to try to fix all of them...
 *                // (do try to fix some though!!!)
 *           }
 *      }
 *
 *      Luckily, you won't have to call very many libterm functions yourself,
 *      so you don't have to partake in this elaborate ritual too often...
 */

/* Ok, I think it's high time I had a definitive policy on what errors use what
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

/* program should try to resolve or report the error
 * used for system call errors
 */
#define FIXABLE_SYS_ERR(name, data) \
	do { \
		curerr.sys_func = name; \
		curerr.ltm_func = __func__; \
		curerr.err_no = errno; \
		error_info_dump(curerr, data, 1); \
		return -1; \
	} while(0);

/* program should try to resolve or report the error
 * used for errors set by a libterm function
 */
#define FIXABLE_LTM_ERR(err, data) \
	do { \
		curerr.sys_func = NULL; \
		curerr.ltm_func = __func__; \
		curerr.err_no = err; \
		error_info_dump(curerr, data, 1); \
		errno = err; \
		return -1; \
	} while(0);

/* program should not try to resolve or report the error
 * used for system call errors
 */
#define FATAL_ERR(name, data) \
	do { \
		curerr.sys_func = name; \
		curerr.ltm_func = __func__; \
		curerr.err_no = errno; \
		error_info_dump(curerr, data, 0); \
		errno = 0; \
		return -1; \
	} while(0);

struct error_info {
	char * sys_func;
	const char * ltm_func;
	int err_no;
};

extern struct error_info curerr;
extern FILE * dump_dest;

extern void error_info_dump(struct error_info, char *, int);

#endif
