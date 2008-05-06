#ifndef CONF_H
#define CONF_H

extern int config_parse_files();
extern char *config_get_entry(char *, char *, char *, char *);
extern int config_interpret_boolean(char *);
extern void config_free();

#endif
