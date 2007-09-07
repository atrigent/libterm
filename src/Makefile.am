AM_CFLAGS = -Wall -Wextra -Werror-implicit-function-declaration
AM_LIBTOOLFLAGS = --silent

if HAVE_UNIX98_FUNCS
if HAVE_POSIX_OPENPT
 AM_CPPFLAGS = -D_XOPEN_SOURCE=600
else
 AM_CPPFLAGS = -D_XOPEN_SOURCE
endif
endif

lib_LTLIBRARIES = libterm.la
libterm_la_SOURCES = bsd.c descriptor.h error.h init.c libterm.h process.c \
trynode.c util.c choosepty.c error.c funcpty.c interface.c ptydev.h unix98.c outputproc.c
