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
libterm_la_SOURCES = bsd.c callbacks.c choosepty.c cursor.c error.c funcpty.c init.c \
		     interface.c outputproc.c process.c trynode.c unix98.c util.c \
		     callbacks.c cursor.h descriptor.h interface.h error.h libterm.h \
		     process.h ptydev.h util.h
