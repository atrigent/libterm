# Process w/ autoconf

AC_PREREQ([2.61])
AC_INIT([libterm], [devel], [lmage11@twcny.rr.com], [libterm])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/init.c])

AM_INIT_AUTOMAKE

AC_PROG_CC

AC_CHECK_HEADERS([sys/ioctl.h])
AC_CHECK_HEADERS([sys/types.h])
AC_CHECK_HEADERS([sys/stat.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([stdio.h])
AC_CHECK_HEADERS([pwd.h])
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([errno.h])
AC_CHECK_HEADERS([libgen.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([utmp.h])
AC_CHECK_HEADERS([time.h])
AC_CHECK_HEADERS([stdint.h])

AC_CONFIG_FILES([Makefile src/Makefile])

AC_OUTPUT
