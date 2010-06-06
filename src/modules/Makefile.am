pkglib_LTLIBRARIES = conf_textfile.la
conf_textfile_la_SOURCES = conf_textfile.c ../textparse.c
conf_textfile_la_LDFLAGS = -module -avoid-version
conf_textfile_la_CPPFLAGS = -DGLOBAL_CONFIG=\"$(sysconfdir)/libterm.conf\" -I$(top_srcdir)/src

if HAVE_GCC_VISIBILITY
conf_textfile_la_CFLAGS = -fvisibility=hidden
endif
