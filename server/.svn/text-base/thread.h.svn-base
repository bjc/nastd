/* $Id: thread.h,v 1.5 2000/03/15 00:09:25 shmit Exp $ */

#ifndef THREAD_H
#	define THREAD_H

#include "conf.h"

#include <pthread.h>

typedef pthread_t thread_id;
typedef pthread_mutex_t mutex_t;

struct _rwlock {
	mutex_t lock;
	int state;
	pthread_cond_t read_sig;
	pthread_cond_t write_sig;
	int blocked_writers;
};
typedef struct _rwlock rw_mutex_t;

struct thread {
	void *(*thread_func)(void *arg);
	pthread_t id;
};
typedef struct thread thread_t;

struct cond {
	pthread_cond_t id;
	mutex_t lock;
};
typedef struct cond cond_t;

int thread_new(thread_t *thread, void *(*thread_func)(void *), void *arg);
void thread_kill(thread_t *thread);
int thread_reload(thread_t *thread);
int mutex_new(mutex_t *lock);
int mutex_lock(mutex_t *lock);
int mutex_unlock(mutex_t *lock);
int rw_mutex_new(rw_mutex_t *lock);
int rw_mutex_read_lock(rw_mutex_t *lock);
int rw_mutex_write_lock(rw_mutex_t *lock);
int rw_mutex_unlock(rw_mutex_t *lock);
int cond_new(cond_t *cond);
int cond_signal(cond_t *cond);
int cond_wait(cond_t *cond);
int cond_timedwait(cond_t *cond, const struct timespec *abstime);
int cond_destroy(cond_t *cond);
#endif
