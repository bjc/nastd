#include "conf.h"
#include "fqm.h"
#include "log.h"
#include "mysqldb.h"
#include "thread.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

RCSID("$Id: fqm.c,v 1.33 2001/10/29 11:17:13 shmit Exp $");

/* Global Variables */
fqm_t *fqm;

request_t *
req_new(int sock, short reqid, REQHANDLER((*req_handler)),
	const char *reqstr, int reqlen)
{
	request_t *tmp;

	tmp = malloc(sizeof(request_t));
	if (tmp == NULL) {
		log_err("Couldn't allocate request: %s.", strerror(errno));
		return NULL;
	}

	tmp->req = malloc((reqlen + 1) * sizeof(char));
	if (tmp->req == NULL) {
		log_err("Couldn't allocate request string: %s.",
			strerror(errno));
		req_delete(tmp);
		return NULL;
	}
	memcpy(tmp->req, reqstr, reqlen);
	tmp->req[reqlen] = '\0';
	tmp->sock = sock;
	tmp->reqid = reqid;
	tmp->req_handler = req_handler;

	return tmp;
}

void
req_delete(request_t *req)
{
	if (req) {
		if (req->req)
			free(req->req);
		free(req);
	}
}

static request_t *
fqm_pop(reqthread_t *req)
{
	fqm_t *fqm;
	request_t *p;

	if (req == NULL || req->fqm == NULL) {
		mutex_unlock(&req->fqm->q_cond->lock);
		return NULL;
	}
	fqm = req->fqm;

	/* Wait for something to arrive on the queue. */
	mutex_lock(&req->fqm->q_cond->lock);
	req->status = IDLE;

	if (fqm->sreq == NULL)
		cond_wait(fqm->q_cond);

	/* Something waiting on the queue. Scarf it off and unlock. */
	p = fqm->sreq;
	if (p == NULL) {
		mutex_unlock(&fqm->q_cond->lock);
		return NULL;
	}
	fqm->sreq = fqm->sreq->next;
	fqm->nitems--;
	req->status = BUSY;
	mutex_unlock(&fqm->q_cond->lock);

	p->next = NULL;
	return p;
}

static void *
fqm_popper(void *arg)
{
	reqthread_t *self;
	request_t *req;

	/* Detach this thread. */
	(void)pthread_detach(pthread_self());

	self = (reqthread_t *)arg;
	for (;;) {
		req = fqm_pop(self);

		/*
		 * Check to see if we were told to die. If we were,
		 * do so.
		 * Note: there's the possibility for the dropping of
		 * a request here. Que sera sera.
		 */
		if (self->dieflag)
			break;
		if (req == NULL)
			continue;
		if (req->req_handler(req, self) == -1)
			log_info("Couldn't handle request");
		req_delete(req);
	}

	log_info("Got a kill flag; dying off.");
	mutex_lock(&self->fqm->t_cond->lock);
	self->status = NOALLOC;
	cond_signal(self->fqm->t_cond);
	mutex_unlock(&self->fqm->t_cond->lock);
	return NULL;
}

static reqthread_t *
fqm_thr_new(fqm_t *fqm)
{
	reqthread_t *tmp;

	tmp = malloc(sizeof(reqthread_t));
	if (tmp == NULL) {
		log_err("Couldn't allocate fallthrough thread: %s.",
			strerror(errno));
		return NULL;
	}

	tmp->arg = NULL;
	tmp->fqm = fqm;
	tmp->dieflag = 0;
	tmp->status = NOALLOC;

	tmp->tid = malloc(sizeof(thread_t));
	if (tmp->tid == NULL) {
		log_err("Couldn't allocate fallthrough thread: %s.",
			strerror(errno));
		free(tmp);
		return NULL;
	}

	if (thread_new(tmp->tid, fqm_popper, tmp)) {
		log_err("Couldn't start fallthrough thread: %s.",
			strerror(errno));
		free(tmp->tid);
		tmp->tid = NULL;
		free(tmp);
		return NULL;
	}
	tmp->status = IDLE;

	return tmp;
}

static int
fqm_thr_wait(fqm_t *fqm)
{
	int deadthreads;
	int i;

	deadthreads = 0;

	for (i = 0; i < fqm->maxitems; i++) {
		if (fqm->tids[i] && fqm->tids[i]->status == NOALLOC) {
			mysqldb_connect_close(fqm->tids[i]->arg);
			if (fqm->tids[i]->tid != NULL)
				free(fqm->tids[i]->tid);
			free(fqm->tids[i]);
			fqm->tids[i] = NULL;
			deadthreads++;
		}
	}

	return deadthreads;
}

fqm_t *
fqm_new(int maxitems)
{
	fqm_t *tmp;
	int i;

	tmp = malloc(sizeof(fqm_t));
	if (tmp == NULL) {
		log_err("Couldn't create fallthrough queue manager: %s.",
			strerror(errno));
		return NULL;
	}

	tmp->q_cond = NULL;
	tmp->t_cond = NULL;
	tmp->tids = NULL;
	tmp->sreq = NULL; tmp->ereq = NULL;
	tmp->maxitems = 0;
	tmp->nitems = 0;

	tmp->q_cond = malloc(sizeof(cond_t));
	if (tmp->q_cond == NULL) {
		log_err("Couldn't allocate queue condition variable: %s.",
			strerror(errno));
		fqm_delete(tmp);
		return NULL;
	}

	if (cond_new(tmp->q_cond)) {
		log_err("Couldn't initialise queue condition variable: %s.",
			strerror(errno));
		fqm_delete(tmp);
		return NULL;
	}

	tmp->t_cond = malloc(sizeof(cond_t));
	if (tmp->t_cond == NULL) {
		log_err("Couldn't allocate queue destroy condition: %s.",
			strerror(errno));
		fqm_delete(tmp);
		return NULL;
	}

	if (cond_new(tmp->t_cond)) {
		log_err("Couldn't initialise queue destroy condition: %s.",
			strerror(errno));
		fqm_delete(tmp);
		return NULL;
	}

	tmp->tids = malloc(maxitems * sizeof(reqthread_t *));
	if (tmp->tids == NULL) {
		log_err("Couldn't allocate fallthrough queue threads: %s.",
			strerror(errno));
		fqm_delete(tmp);
		return NULL;
	}

	/* Init these so fqm_delete won't break. */
	for (i = 0; i < maxitems; i++)
		tmp->tids[i] = NULL;

	/* We cannot adjust max items while threads are busy, so lock. */
	mutex_lock(&tmp->q_cond->lock);
	for (i = 0; i < maxitems; i++) {
		tmp->tids[i] = fqm_thr_new(tmp);
		if (tmp->tids[i] == NULL) {
			mutex_unlock(&tmp->q_cond->lock);
			fqm_delete(tmp);
			return NULL;
		}
		tmp->maxitems++;
	}
	mutex_unlock(&tmp->q_cond->lock);

	/* XXX: this may need a lock. */
	fqm = tmp;
	return tmp;
}

/*
 * Make sure all threads in this fqm are idle. This is an expensive
 * operation, but that's okay, since this code should only be run when
 * a client has closed the connection, so we can take as long as we want.
 *
 * The queue is locked when coming in to this routine and leaving it.
 */
void
verify_idle_threads(fqm_t *fqm)
{
	int i, busy_thread;

	do {
		busy_thread = 0;
		for (i = 0; i < fqm->maxitems; i++) {
			if (fqm->tids[i]->status == BUSY)
				busy_thread = 1;
		}

		/*
		 * This is the shitty part.
		 * We unlock the queue and sleep for a bit to give other
		 * threads a chance to finish what they're doing.
		 */
		if (busy_thread) {
			mutex_unlock(&fqm->q_cond->lock);
			sleep(1);
			mutex_lock(&fqm->q_cond->lock);
		}
	} while (busy_thread);
}

void
fqm_delete(fqm_t *fqm)
{
	int i;

	if (fqm->tids) {
		/* Tell all threads to quit. */
		for (i = 0; i < fqm->maxitems; i++) {
			if (fqm->tids[i] != NULL)
				fqm->tids[i]->dieflag = 1;
		}

		/*
		 * Lock queue waiting condition to ensure that no
		 * threads miss this signal. The threads MUST be idle
		 * in order to guarantee this signal is recieved.
		 */
		mutex_lock(&fqm->q_cond->lock);
		verify_idle_threads(fqm);
		pthread_cond_broadcast(&fqm->q_cond->id);
		mutex_unlock(&fqm->q_cond->lock);

		/* Now wait for them to die off. */
		i = 0;
		while (i < fqm->maxitems) {
			mutex_lock(&fqm->t_cond->lock);
			i += fqm_thr_wait(fqm);
			if (i < fqm->maxitems)
				cond_wait(fqm->t_cond);
			mutex_unlock(&fqm->t_cond->lock);
		}
		free(fqm->tids);
		fqm->tids = NULL;
	}

	/*
	 * At this point all fqm threads should be dead.
	 */
	if (fqm->t_cond) {
		(void)cond_destroy(fqm->t_cond);
		free(fqm->t_cond);
		fqm->t_cond = NULL;
	}

	if (fqm->q_cond) {
		(void)cond_destroy(fqm->q_cond);
		free(fqm->q_cond);
		fqm->q_cond = NULL;
	}

	free(fqm);
}

int
fqm_changemaxitems(fqm_t *fqm, int maxitems)
{
	reqthread_t **new_tidpool;
	int i, j;

	/*
	 * This code broken. Silently fail here.
	 */
	return 0;

	if (fqm->maxitems == maxitems)
		return 0;

	mutex_lock(&fqm->q_cond->lock);
	/*
	 * Make sure all threads are busy. We can't go changing
	 * data under them.
	 */
	verify_idle_threads(fqm);
	new_tidpool = malloc(maxitems * sizeof(reqthread_t *));
	if (new_tidpool == NULL) {
		mutex_unlock(&fqm->q_cond->lock);
		log_err("Couldn't allocate new FQM thread pool: %s.",
			strerror(errno));
		return -1;
	}

	/* Compress old TID pool. */
	for (j = 0, i = 0; i < fqm->maxitems && j < maxitems; i++) {
		if (fqm->tids[i]->status != NOALLOC) {
			new_tidpool[j] = fqm->tids[i];
			j++;
		}
	}

	if (fqm->maxitems < maxitems) {
		/* Add more threads. */
		mutex_lock(&fqm->q_cond->lock);
		for (i = fqm->maxitems; i < maxitems; i++) {
			new_tidpool[i] = fqm_thr_new(fqm);
			if (new_tidpool[i] == NULL) {
				free(new_tidpool);
				mutex_unlock(&fqm->q_cond->lock);
				return -1;
			}
		}
		mutex_unlock(&fqm->q_cond->lock);
	} else if (fqm->maxitems > maxitems) {
		/* Kill some threads. */
		int deadthreads;

		/* Tell them to die, then wake 'em up. */
		for (i = maxitems; i < fqm->maxitems; i++)
			fqm->tids[i]->dieflag = 1;
		pthread_cond_broadcast(&fqm->q_cond->id);

		deadthreads = 0;
		while (deadthreads < fqm->maxitems - maxitems) {
			mutex_lock(&fqm->t_cond->lock);
			deadthreads += fqm_thr_wait(fqm);
			if (deadthreads < fqm->maxitems - maxitems)
				cond_wait(fqm->t_cond);
			mutex_unlock(&fqm->t_cond->lock);
		}
	}

	free(fqm->tids);
	fqm->tids = new_tidpool;
	fqm->maxitems = maxitems;
	mutex_unlock(&fqm->q_cond->lock);
	return 0;
}

int
fqm_push(fqm_t *fqm, request_t *req)
{
	if (fqm->nitems == fqm->maxitems) {
		log_err("Too many items on the request queue.");
		return -1;
	}

	/* Lock the queue and add the item. */
	mutex_lock(&fqm->q_cond->lock);

	req->next = NULL;
	if (fqm->sreq == NULL)
		fqm->sreq = req;
	else
		fqm->ereq->next = req;
	fqm->ereq = req;
	fqm->nitems++;

	/* Unlock the queue and signal that there's an item on it. */
	cond_signal(fqm->q_cond);
	mutex_unlock(&fqm->q_cond->lock);
	return 0;
}
