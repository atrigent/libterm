#ifndef PROCESS_H
#define PROCESS_H

#include <signal.h>

extern int spawn(char *, FILE *);
extern int set_handler(int, void (*)(int, siginfo_t *, void *));
extern void dontfearthereaper(int, siginfo_t *, void *);

extern struct sigaction oldaction;
extern FILE * pipefiles[];

#endif
