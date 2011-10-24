#ifndef LIBTERM_CONF_MODULE_H
#define LIBTERM_CONF_MODULE_H

#ifdef WIN32
#  define extern extern __declspec(dllimport)
#endif

#include <libterm_module.h>

extern int ltm_config_mod_set_entry(enum ltm_moduleclass, char *, char *, char *);

#ifdef WIN32
#  undef extern
#endif

#endif
