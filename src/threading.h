#ifndef THREADING_H
#define THREADING_H

/* macros to make pthread stuff do nothing if pthread isn't supported */
#ifdef HAVE_LIBPTHREAD
# include <pthread.h>
#else
# define pthread_create(thread, attr, func, data) 0
# define pthread_mutexattr_settype(attr, type) 0
# define pthread_mutex_init(mutex, attr) 0
# define pthread_mutexattr_destroy(attr) 0
# define pthread_mutex_destroy(mutex) 0
# define pthread_mutexattr_init(attr) 0
# define pthread_mutex_unlock(mutex) 0
# define pthread_mutex_lock(mutex) 0
# define pthread_join(thread, val) 0
# define pthread_t int
# define pthread_mutex_t int
# define pthread_mutexattr_t int
# define PTHREAD_MUTEX_RECURSIVE 0
#endif

#define EXIT_THREAD 0
#define NEW_TERM 1
#define DEL_TERM 2

/* handling errors from pthread calls */

#define PTHREAD_CALL(func, args, data) \
	if((errno = func args)) \
		SYS_ERR(#func, data);

#define PTHREAD_CALL_PTR(func, args, data) \
	if((errno = func args)) \
		SYS_ERR_PTR(#func, data);

#define PTHREAD_CALL_THR(func, args, data) \
	if((errno = func args)) \
		SYS_ERR_THR(#func, data);

/* locking and unlocking */

#define LOCK_BIG_MUTEX \
	PTHREAD_CALL(pthread_mutex_lock, (&the_big_mutex), NULL)

#define UNLOCK_BIG_MUTEX \
	PTHREAD_CALL(pthread_mutex_unlock, (&the_big_mutex), NULL)

#define LOCK_BIG_MUTEX_THR \
	PTHREAD_CALL_THR(pthread_mutex_lock, (&the_big_mutex), NULL)

#define UNLOCK_BIG_MUTEX_THR \
	PTHREAD_CALL_THR(pthread_mutex_unlock, (&the_big_mutex), NULL)

#define LOCK_BIG_MUTEX_PTR \
	PTHREAD_CALL_PTR(pthread_mutex_lock, (&the_big_mutex), NULL)

#define UNLOCK_BIG_MUTEX_PTR \
	PTHREAD_CALL_PTR(pthread_mutex_unlock, (&the_big_mutex), NULL)

extern pthread_mutex_t the_big_mutex;
extern void *watch_for_events();
extern char threading;

#endif
