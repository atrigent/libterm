#ifndef PROCESS_H
#define PROCESS_H

#include <signal.h>

extern int spawn(char *, FILE *);
extern int setup_signal(int, void (*)(int, siginfo_t *, void *));
extern void dontfearthereaper(int, siginfo_t *, void *);

#endif
