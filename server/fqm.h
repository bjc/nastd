#ifndef FQM_H
#define FQM_H

#include "thread.h"

#define REQHANDLER(func) int func(struct _request_t *req, reqthread_t *self)

enum _status_t { IDLE, BUSY, NOALLOC };
typedef enum _status_t status_t;

struct _reqthread_t {
	struct _fqm_t *fqm;
	thread_t *tid;
	void *arg;
	int dieflag;
	status_t status;
};
typedef struct _reqthread_t reqthread_t;

struct _request_t {
	REQHANDLER((*req_handler));
	char *req;
	int sock;
	short reqid;
	struct _request_t *next;
};
typedef struct _request_t request_t;

struct _fqm_t {
	cond_t *q_cond;
	cond_t *t_cond;
	request_t *sreq, *ereq;
	reqthread_t **tids;
	int maxitems;
	int nitems;
};
typedef struct _fqm_t fqm_t;

request_t *req_new(int sock, short reqid, REQHANDLER((*req_handler)),
		   const char *reqstr, int reqlen);
void req_delete(request_t *req);

fqm_t *fqm_new(int maxitems);
void fqm_delete(fqm_t *fqm);
int fqm_changemaxitems(fqm_t *fqm, int maxitems);
int fqm_push(fqm_t *fqm, request_t *req);

#endif
