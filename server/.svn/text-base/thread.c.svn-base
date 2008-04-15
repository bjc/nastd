#include "conf.h"
#include "log.h"
#include "thread.h"

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

RCSID("$Id: thread.c,v 1.6 2000/03/21 19:18:24 shmit Exp $");

int
thread_new(thread_t *thread, void *(*thread_func)(void *), void *arg)
{
	int rc;

	thread->thread_func = thread_func;
	rc = pthread_create(&thread->id, NULL, thread_func, arg);
	if (rc != 0) {
		log_err("Couldn't create thread: %s.\n", strerror(rc));
		return -1;
	}
	return 0;
}

void
thread_kill(thread_t *thread)
{
}

int
thread_reload(thread_t *thread)
{
	return 0;
}

int
mutex_new(mutex_t *lock)
{
	return pthread_mutex_init(lock, NULL);
}

int
mutex_delete(mutex_t *lock)
{
	return pthread_mutex_destroy(lock);
}

int
mutex_lock(mutex_t *lock)
{
	return pthread_mutex_lock(lock);
}

int
mutex_unlock(mutex_t *lock)
{
	return pthread_mutex_unlock(lock);
}

int
rw_mutex_new(rw_mutex_t *lock)
{
	int rc;

	rc = mutex_new(&lock->lock);
	if (rc)
		return rc;
	rc = pthread_cond_init(&lock->read_sig, NULL);
	if (rc) {
		mutex_delete(&lock->lock);
		return rc;
	}
	rc = pthread_cond_init(&lock->write_sig, NULL);
	if (rc) {
		mutex_delete(&lock->lock);
		pthread_cond_destroy(&lock->read_sig);
		return rc;
	}
	lock->state = 0;
	lock->blocked_writers = 0;

	return 0;
}

int
rw_mutex_read_lock(rw_mutex_t *lock)
{
	int rc;

	if (lock == NULL)
		return EINVAL;

	rc = mutex_lock(&lock->lock);
	if (rc)
		return rc;

	/* Make sure the writers go before the readers. */
	while (lock->blocked_writers || lock->state < 0) {
		rc = pthread_cond_wait(&lock->read_sig, &lock->lock);
		if (rc) {
			mutex_unlock(&lock->lock);
			return rc;
		}
	}
	lock->state++;
	mutex_unlock(&lock->lock);

	return rc;
}

int
rw_mutex_write_lock(rw_mutex_t *lock)
{
	int rc;

	if (lock == NULL)
		return EINVAL;

	rc = mutex_lock(&lock->lock);
	if (rc)
		return rc;

	/* Wait for no readers on the lock. */
	while (lock->state != 0) {
		lock->blocked_writers++;
		rc = pthread_cond_wait(&lock->write_sig, &lock->lock);
		lock->blocked_writers--;
		if (rc) {
			mutex_unlock(&lock->lock);
			return rc;
		}
	}
	lock->state = -1;

	mutex_unlock(&lock->lock);
	return rc;
}

int
rw_mutex_unlock(rw_mutex_t *lock)
{
	int rc;

	if (lock == NULL)
		return EINVAL;

	rc = mutex_lock(&lock->lock);
	if (rc)
		return rc;

	if (lock->state > 0) {
		/* We have an open read lock. */
		if (--lock->state == 0 && lock->blocked_writers)
			rc = pthread_cond_signal(&lock->write_sig);
	} else if (lock->state < 0) {
		/* We have an open writer lock. */
		lock->state = 0;
		if (lock->blocked_writers)
			rc = pthread_cond_signal(&lock->write_sig);
		else
			rc = pthread_cond_broadcast(&lock->read_sig);
	} else
		rc = EINVAL;

	mutex_unlock(&lock->lock);
	return rc;
}

int
cond_new(cond_t *cond)
{
	int rc;

	if (cond == NULL)
		return EINVAL;

	rc = pthread_cond_init(&cond->id, NULL);
	if (rc)
		return rc;

	rc = mutex_new(&cond->lock);
	if (rc)
		pthread_cond_destroy(&cond->id);

	return rc;
}

int
cond_signal(cond_t *cond)
{
	return pthread_cond_signal(&cond->id);
}

int
cond_wait(cond_t *cond)
{
	return pthread_cond_wait(&cond->id, &cond->lock);
}

int
cond_timedwait(cond_t *cond, const struct timespec *abstime)
{
	return pthread_cond_timedwait(&cond->id, &cond->lock, abstime);
}

int
cond_destroy(cond_t *cond)
{
	return pthread_cond_destroy(&cond->id);
}
