#ifndef THREADING_H
#define THREADING_H

/* macros to make pthread stuff do nothing if pthread isn't supported */
#ifdef HAVE_LIBPTHREAD
# include <pthread.h>

# define EXIT_THREAD 0
# define NEW_TERM 1
# define DEL_TERM 2

/* handling errors from pthread calls */

# define PTHREAD_CALL(func, args, data, label) \
	if((errno = func args)) \
		SYS_ERR(#func, data, label)

# define PTHREAD_CALL_PTR(func, args, data, label) \
	if((errno = func args)) \
		SYS_ERR_PTR(#func, data, label)

# define PTHREAD_CALL_THR(func, args, data, label) \
	if((errno = func args)) \
		SYS_ERR_THR(#func, data, label)

/* locking and unlocking */

# define MUTEX_LOCK(mutex, label) \
	PTHREAD_CALL(pthread_mutex_lock, (&mutex), NULL, label)

# define MUTEX_UNLOCK(mutex, label) \
	PTHREAD_CALL(pthread_mutex_unlock, (&mutex), NULL, label)

# define MUTEX_LOCK_PTR(mutex, label) \
	PTHREAD_CALL_PTR(pthread_mutex_lock, (&mutex), NULL, label)

# define MUTEX_UNLOCK_PTR(mutex, label) \
	PTHREAD_CALL_PTR(pthread_mutex_unlock, (&mutex), NULL, label)

# define MUTEX_LOCK_THR(mutex, label) \
	PTHREAD_CALL_THR(pthread_mutex_lock, (&mutex), NULL, label)

# define MUTEX_UNLOCK_THR(mutex, label) \
	PTHREAD_CALL_THR(pthread_mutex_unlock, (&mutex), NULL, label)

/* locking and unlocking of the "big mutex" */

# define LOCK_BIG_MUTEX \
	MUTEX_LOCK(the_big_mutex, before_anything)

# define UNLOCK_BIG_MUTEX \
	MUTEX_UNLOCK(the_big_mutex, before_anything)

# define LOCK_BIG_MUTEX_THR \
	MUTEX_LOCK_THR(the_big_mutex, after_alloc)

# define UNLOCK_BIG_MUTEX_THR \
	MUTEX_UNLOCK_THR(the_big_mutex, after_alloc)

# define LOCK_BIG_MUTEX_PTR \
	MUTEX_LOCK_PTR(the_big_mutex, before_anything)

# define UNLOCK_BIG_MUTEX_PTR \
	MUTEX_UNLOCK_PTR(the_big_mutex, before_anything)

extern pthread_mutex_t the_big_mutex;
extern void *watch_for_events();
extern pthread_t watchthread;
extern char threading;

#else /* HAVE_LIBPTHREAD */

# define PTHREAD_CALL(func, args, data, label)
# define PTHREAD_CALL_PTR(func, args, data, label)
# define PTHREAD_CALL_THR(func, args, data, label)

# define MUTEX_LOCK(mutex, label)
# define MUTEX_UNLOCK(mutex, label)
# define MUTEX_LOCK_PTR(mutex, label)
# define MUTEX_UNLOCK_PTR(mutex, label)
# define MUTEX_LOCK_THR(mutex, label)
# define MUTEX_UNLOCK_THR(mutex, label)

# define LOCK_BIG_MUTEX
# define UNLOCK_BIG_MUTEX
# define LOCK_BIG_MUTEX_THR
# define UNLOCK_BIG_MUTEX_THR
# define LOCK_BIG_MUTEX_PTR
# define UNLOCK_BIG_MUTEX_PTR

#endif /* HAVE_LIBPTHREAD */

#endif
