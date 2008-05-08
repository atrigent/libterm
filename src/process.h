#ifndef PROCESS_H
#define PROCESS_H

#include <signal.h>

extern int spawn(char *, FILE *);
extern void dontfearthereaper(int, siginfo_t *, void *);
extern int reload_handler(int, void (*)(int, siginfo_t *, void *));
extern int set_handler_struct(int, void (*)(int, siginfo_t *, void *), struct sigaction *);

extern struct sigaction oldaction;
extern FILE *pipefiles[];

#endif
