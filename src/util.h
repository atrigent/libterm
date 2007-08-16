#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>

extern short numlen(unsigned long long, char);
extern int file_check_type(char *, char *, dev_t *);
extern int is_chrdev(char *, dev_t);
extern char * get_tmp_dir();

#endif
