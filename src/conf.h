#ifndef CONF_H
#define CONF_H

enum moduleclass {
	LIBTERM
};

extern int config_init();
extern char *config_get_entry(enum moduleclass, char *, char *, char *);
extern int config_interpret_boolean(char *);
extern void config_free();

#endif
