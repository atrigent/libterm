#ifndef THREADING_H
#define THREADING_H

/* macros to make pthread stuff do nothing if pthread isn't supported */
#ifdef HAVE_LIBPTHREAD
# include <pthread.h>
#elif
# define pthread_create(thread, attr, func, data)
# define pthread_join(thread, val)
# define pthread_t int
#endif

#define EXIT_THREAD 0
#define NEW_TERM 1
#define DEL_TERM 2

extern void *watch_for_events();

extern char threading;

#endif
