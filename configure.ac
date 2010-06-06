# Process w/ autoconf

dnl Autoconf requirements
AC_PREREQ(2.63)
dnl Boilerplate
AC_INIT([libterm],[devel],[lmage11@twcny.rr.com],[libterm])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/init.c])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE

dnl Options
AC_MSG_CHECKING([for default configuration module])
AC_ARG_WITH([default-conf-module],
	[AS_HELP_STRING([--with-default-conf-module],
		[which configuration module to load by default (default is textfile)])],
	[], [with_default_conf_module=textfile])
AC_MSG_RESULT([$with_default_conf_module])
AC_DEFINE_UNQUOTED([DEFAULT_CONF_MODULE], ["$with_default_conf_module"], [Configuration module to load by default])

dnl Checks for programs
LT_CONFIG_LTDL_DIR([libltdl])
LT_INIT([dlopen])
LTDL_INIT([recursive])

if test "x$with_included_ltdl" = xyes; then
	AC_CONFIG_FILES([libltdl/Makefile])
	LTDLDIR=libltdl
fi

AC_SUBST([LIBLTDL])
AC_SUBST([LTDLINCL])
AC_SUBST([LTDLDIR])

AC_PROG_CC
AM_PROG_CC_C_O

dnl Checks for header files
AC_HEADER_STDC

if test $ac_cv_header_stdc = no; then
	AC_MSG_ERROR([Cannot find the standard ANSI C headers! Please install them or upgrade your system.])
fi

AC_HEADER_MAJOR
dnl Is it fair to assume that the other ioctl macros will be in the same place as TIOCGWINSZ?
AC_HEADER_TIOCGWINSZ
AC_CHECK_HEADERS(
	[sys/ioctl.h \
	sys/types.h \
	sys/stat.h \
	unistd.h \
	stdio.h \
	pwd.h \
	string.h \
	stdlib.h \
	errno.h \
	libgen.h \
	fcntl.h \
	utmp.h \
	time.h],
	[],
	[AC_MSG_ERROR([The required header $ac_header was not found!])]
)

dnl Checks for types
AC_TYPE_PID_T

dnl Checks for structures

dnl Checks for compiler characteristics

dnl check whether GCC supports visibility
save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -fvisibility=hidden"

AC_MSG_CHECKING(whether $CC accepts -fvisibility=hidden)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM()], [gcc_visibility="yes"], [gcc_visibility="no"])

CFLAGS="$save_CFLAGS"

AM_CONDITIONAL([HAVE_GCC_VISIBILITY], [test $gcc_visibility = yes])
AC_MSG_RESULT($gcc_visibility)

if test $gcc_visibility = yes; then
	AC_DEFINE([HAVE_GCC_VISIBILITY], [1], [Define to 1 if the GCC installed on your system supports controlling symbol visibility (the -fvisibility flag)])
fi

dnl Checks for library functions
AC_CHECK_LIB([pthread], [pthread_create], 
		[AC_DEFINE([HAVE_LIBPTHREAD], [1], [Define to 1 if you have the 'pthread' library (-lpthread).])],
		[AC_MSG_WARN([The pthread library was not found; libterm will be built without threading support.])])

AM_CONDITIONAL([HAVE_PTHREAD], [test $ac_cv_lib_pthread_pthread_create = yes])

dnl Checks for pty functions
AC_CHECK_FUNCS([posix_openpt getpt], [break])
AC_CHECK_FUNCS([open grantpt unlockpt ptsname])

unix98_funcs=no

if	(
		test $ac_cv_func_posix_openpt = yes ||
		test $ac_cv_func_getpt = yes ||
		test $ac_cv_func_open = yes
	) &&
	test $ac_cv_func_grantpt = yes &&
	test $ac_cv_func_unlockpt = yes &&
	test $ac_cv_func_ptsname = yes; then
	AC_DEFINE([HAVE_UNIX98_FUNCS], [1], [Define to 1 if you have the Unix98 PTY functions, i.e. posix_openpt, getpt, or open and grantpt, unlockpt, and ptsname])
	unix98_funcs=yes
else
	AC_SEARCH_LIBS([openpty], [util bsd])
	AC_CHECK_FUNCS([openpty])

	if test $ac_cv_func_openpty = no; then
		AC_MSG_ERROR([Could not find any system pseudoterminal facilities!])
	fi
fi

AM_CONDITIONAL([HAVE_UNIX98_FUNCS], [test $unix98_funcs = yes])
AM_CONDITIONAL([HAVE_POSIX_OPENPT], [test $ac_cv_func_posix_openpt = yes])

dnl Checks for system services

CFLAGS="-Wall -Wextra -Werror-implicit-function-declaration $CFLAGS"
AC_SUBST([libtoolflags], [--silent])
AC_CONFIG_FILES([Makefile src/Makefile src/modules/Makefile])

AC_OUTPUT
