AM_LIBTOOLFLAGS = @libtoolflags@

lib_LTLIBRARIES = libterm.la
libterm_la_SOURCES = callbacks.c cursor.c error.c init.c window.c \
		     io.c process.c bitarr.c pty.c conf.c linkedlist.c \
		     hashtable.c idarr.c screen.c rangeset.c module.c \
		     callbacks.h cursor.h descriptor.h error.h window.h libterm.h \
		     process.h ptydev.h bitarr.h threading.h conf.h \
		     linkedlist.h hashtable.h textparse.h idarr.h screen.h \
		     rangeset.h module.h
libterm_la_CPPFLAGS = @LTDLINCL@ -DMODULE_PATH=\"$(pkglibdir)\"
libterm_la_LIBADD = @LIBLTDL@
libterm_la_CFLAGS = -Wno-unused-label

if HAVE_UNIX98_FUNCS
if HAVE_POSIX_OPENPT
libterm_la_CPPFLAGS += -D_XOPEN_SOURCE=600
else
libterm_la_CPPFLAGS += -D_XOPEN_SOURCE
endif
endif

if HAVE_GCC_VISIBILITY
libterm_la_CFLAGS += -fvisibility=hidden
endif

if HAVE_PTHREAD
libterm_la_CFLAGS += -pthread
endif

SUBDIRS = modules
