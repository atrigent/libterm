# Process w/ autoconf

# Autoconf requirements
AC_PREREQ([2.61])
# Boilerplate
AC_INIT([libterm], [devel], [lmage11@twcny.rr.com], [libterm])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/init.c])
AM_INIT_AUTOMAKE

# Options

# Checks for programs
AC_PROG_LIBTOOL
AC_PROG_CC

# Checks for libraries
AC_CHECK_LIB([util], [openpty])

# Checks for header files
AC_HEADER_STDC
AC_HEADER_MAJOR
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

# Checks for types

# Checks for structures

# Checks for compiler characteristics

# Checks for library functions
AC_CHECK_FUNCS([posix_openpt])
AC_CHECK_FUNCS([getpt])
AC_CHECK_FUNCS([openpty])
AC_CHECK_FUNCS([grantpt])
AC_CHECK_FUNCS([unlockpt])
AC_CHECK_FUNCS([ptsname])

# Checks for system services

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])

AC_OUTPUT
