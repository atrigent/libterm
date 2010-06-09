#ifndef CONF_H
#define CONF_H

extern int config_init();

extern char *config_get_entry(enum moduleclass, char *, char *, char *);
extern char *config_get_mid_entry(int, char *, char *);

extern int config_set_mid_entry(int, char *, char *);

extern int config_interpret_boolean(char *);

extern void config_free();

#endif
