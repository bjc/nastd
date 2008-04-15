#include "conf.h"
#include "thread.h"

#include <unistd.h>

#ifdef THREADSAFECLIENT
#include <pthread.h>
#endif

#ifdef THREADSAFECLIENT
short
thread_id()
{
	short i;

	i = ((unsigned int)pthread_self() & 0xff) | (getpid() << 8);
	return i;
}

int
_nast_mutex_new(_nast_mutex_t *lock)
{
	return pthread_mutex_init(lock, NULL);
}

void
_nast_mutex_delete(_nast_mutex_t *lock)
{
	(void)pthread_mutex_destroy(lock);
}

int
_nast_mutex_lock(_nast_mutex_t *lock)
{
	return pthread_mutex_lock(lock);
}

int
_nast_mutex_unlock(_nast_mutex_t *lock)
{
	return pthread_mutex_unlock(lock);
}
#else /* THREADSAFECLIENT */
short
thread_id()
{
	return getpid();
}

int
_nast_mutex_new(_nast_mutex_t *lock)
{
	return 0;
}

void
_nast_mutex_delete(_nast_mutex_t *lock)
{
	return;
}

int
_nast_mutex_lock(_nast_mutex_t *lock)
{
	return 0;
}

int
_nast_mutex_unlock(_nast_mutex_t *lock)
{
	return 0;
}
#endif /* THREADSAFECLIENT */
