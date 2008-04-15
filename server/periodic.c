#include "conf.h"
#include "cdb.h"
#include "log.h"
#include "memdb.h"
#include "mysqldb.h"
#include "thread.h"
#include "periodic.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

RCSID("$Id: periodic.c,v 1.9 2000/05/17 21:52:34 shmit Exp $");

struct _per_thread_t {
	cond_t *cond;
	thread_t *tid;
};
typedef struct _per_thread_t per_thread_t;

static void *
periodic_looper(void *arg)
{
	per_thread_t *self;
	int count;

	self = (per_thread_t *)arg;
	pthread_detach(pthread_self());

	count = 0;
	for (;;) {
		struct timeval tv;
		struct timespec ts;

		gettimeofday(&tv, NULL);
		memset(&ts, 0, sizeof(ts));
		ts.tv_sec = tv.tv_sec + 1;
		(void)cond_timedwait(self->cond, &ts);

		count++;
		if (count == PERIODICITY) {
			cdb_periodic();
			count = 0;
		}

		cdb_collate();
		mysqldb_collate();
		memdb_collate();
	}
	return NULL;
}

void
periodic_delete(per_thread_t *thr)
{
	if (thr == NULL)
		return;

	if (thr->cond) {
		cond_destroy(thr->cond);
		free(thr->cond);
		thr->cond = NULL;
	}

	if (thr->tid) {
		free(thr->tid);
		thr->tid = NULL;
	}
	free(thr);
}

int
periodic_new()
{
	per_thread_t *per_thread;
	int rc;

	per_thread = malloc(sizeof(per_thread_t));
	if (per_thread == NULL) {
		log_err("Couldn't allocate periodic thread: %s.",
			strerror(errno));
		return -1;
	}
	per_thread->cond = NULL;
	per_thread->tid = NULL;

	per_thread->cond = malloc(sizeof(cond_t));
	if (per_thread->cond == NULL) {
		log_err("Couldn't allocate periodic condition: %s.",
			strerror(errno));
		periodic_delete(per_thread);
		return -1;
	}

	rc = cond_new(per_thread->cond);
	if (rc) {
		log_err("Couldn't initialise periodic condition: %s.",
			strerror(rc));
		periodic_delete(per_thread);
		return -1;
	}

	per_thread->tid = malloc(sizeof(thread_t));
	if (per_thread->tid == NULL) {
		log_err("Couldn't allocate periodic thread: %s.",
			strerror(rc));
		periodic_delete(per_thread);
		return -1;
	}

	rc = thread_new(per_thread->tid, periodic_looper, per_thread);
	if (rc) {
		log_err("Couldn't start periodic thread: %s.",
			strerror(rc));
		periodic_delete(per_thread);
		return -1;
	}

	return 0;
}
