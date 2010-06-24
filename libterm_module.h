#ifndef LIBTERM_MODULE_H
#define LIBTERM_MODULE_H

enum ltm_moduleclass {
	LTM_MOD_LIBTERM,
	LTM_MOD_CONF
};

extern const char *const ltm_classes[];

extern int config_set_module_entry(char *, char *);
extern char *config_get_module_entry(char *, char *);

#endif
