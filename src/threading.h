#ifndef THREADING_H
#define THREADING_H

/* macros to make pthread stuff do nothing if pthread isn't supported */
#ifdef HAVE_LIBPTHREAD
# include <pthread.h>

# define EXIT_THREAD 0
# define NEW_TERM 1
# define DEL_TERM 2

/* handling errors from pthread calls */

# define PTHREAD_CALL(func, args, data) \
	if((errno = func args)) \
		SYS_ERR(#func, data);

# define PTHREAD_CALL_PTR(func, args, data) \
	if((errno = func args)) \
		SYS_ERR_PTR(#func, data);

# define PTHREAD_CALL_THR(func, args, data) \
	if((errno = func args)) \
		SYS_ERR_THR(#func, data);

/* locking and unlocking */

# define LOCK_BIG_MUTEX \
	PTHREAD_CALL(pthread_mutex_lock, (&the_big_mutex), NULL)

# define UNLOCK_BIG_MUTEX \
	PTHREAD_CALL(pthread_mutex_unlock, (&the_big_mutex), NULL)

# define LOCK_BIG_MUTEX_THR \
	PTHREAD_CALL_THR(pthread_mutex_lock, (&the_big_mutex), NULL)

# define UNLOCK_BIG_MUTEX_THR \
	PTHREAD_CALL_THR(pthread_mutex_unlock, (&the_big_mutex), NULL)

# define LOCK_BIG_MUTEX_PTR \
	PTHREAD_CALL_PTR(pthread_mutex_lock, (&the_big_mutex), NULL)

# define UNLOCK_BIG_MUTEX_PTR \
	PTHREAD_CALL_PTR(pthread_mutex_unlock, (&the_big_mutex), NULL)

extern pthread_mutex_t the_big_mutex;
extern void *watch_for_events();
extern char threading;

#else /* HAVE_LIBPTHREAD */

# define PTHREAD_CALL(func, args, data)
# define PTHREAD_CALL_PTR(func, args, data)
# define PTHREAD_CALL_THR(func, args, data)

# define LOCK_BIG_MUTEX
# define UNLOCK_BIG_MUTEX
# define LOCK_BIG_MUTEX_THR
# define UNLOCK_BIG_MUTEX_THR
# define LOCK_BIG_MUTEX_PTR
# define UNLOCK_BIG_MUTEX_PTR

#endif /* HAVE_LIBPTHREAD */

#endif
