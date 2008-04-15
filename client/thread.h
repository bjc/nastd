/* $Id: thread.h,v 1.5 2000/09/13 20:21:30 shmit Exp $ */

#ifndef THREAD_H
#	define THREAD_H

#ifdef THREADSAFECLIENT
#include <pthread.h>

typedef pthread_mutex_t _nast_mutex_t;
#else
typedef int _nast_mutex_t;
#endif

short thread_id();
int _nast_mutex_new(_nast_mutex_t *lock);
void _nast_mutex_delete(_nast_mutex_t *lock);
int _nast_mutex_lock(_nast_mutex_t *lock);
int _nast_mutex_unlock(_nast_mutex_t *lock);
#endif
