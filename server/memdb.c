#include "conf.h"
#include "config.h"
#include "array.h"
#include "log.h"
#include "md5.h"
#include "memdb.h"
#include "thread.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

RCSID("$Id: memdb.c,v 1.27 2001/10/04 23:57:10 shmit Exp $");

#define HASHSIZE 1024

extern time_t cdb_mtime;

enum _cachetype_t { C_DEL, C_UPD, C_TRYVAL, C_EXPIRE };
typedef enum _cachetype_t cachetype_t;

struct _mement_t {
	char *key;
	int keylen;
	array_t *vals;
	time_t *upd_time;
	cachetype_t type;
	void *typeval;
};
typedef struct _mement_t mement_t;

struct _cachent_t {
	mement_t ent;
	struct _cachent_t *next;
};
typedef struct _cachent_t cachent_t;

struct _counter_t {
	float onemin, fivemin, fifteenmin;
};
typedef struct _counter_t counter_t;

extern short db_fieldcount;

static cachent_t **memdb = NULL;
static int memdbsize;
static rw_mutex_t *memdb_lk = NULL;

static int query_sec, add_sec, del_sec, upd_sec;
static counter_t queryrate;
static counter_t addrate;
static counter_t delrate;
static counter_t updrate;

static unsigned int
hashkey(const unsigned char *key, int keylen)
{
	unsigned char md5hash[32];
	unsigned int rc;

	memset(md5hash, 0, sizeof(md5hash));
	md5_calc(md5hash, key, keylen, sizeof(md5hash));
	memcpy(&rc, md5hash, sizeof(rc));
	return (rc % HASHSIZE);
}

static void
ent_delete(mement_t *ent)
{
	if (ent == NULL)
		return;

	if (ent->key != NULL)
		free(ent->key);
	if (ent->vals != NULL)
		array_delete(ent->vals);
	if (ent->upd_time != NULL)
		free(ent->upd_time);
	if (ent->typeval != NULL)
		free(ent->typeval);
}

int
memdb_new()
{
	cachent_t **newdb;
	int i;
	int rc;

	log_info("Initialising memdb interface.");

	if (memdb_lk == NULL) {
		memdb_lk = malloc(sizeof(rw_mutex_t));
		if (memdb_lk == NULL) {
			log_err("Couldn't allocate memdb mutex: %s.",
				strerror(errno));
			return -1;
		}
		rc = rw_mutex_new(memdb_lk);
		if (rc) {
			log_err("Couldn't initialise memdb mutex: %s.",
				strerror(rc));
			return -1;
		}
	}

	newdb = malloc(HASHSIZE * sizeof(cachent_t *));
	if (newdb == NULL) {
		log_err("Couldn't allocate new memdb: %s.", strerror(errno));
		return -1;
	}

	for (i = 0; i < HASHSIZE; i++)
		newdb[i] = NULL;

	rw_mutex_write_lock(memdb_lk);
	if (memdb != NULL)
		memdb_delete(memdb);
	memdb = newdb;
	rw_mutex_unlock(memdb_lk);

	query_sec = add_sec = del_sec = upd_sec = 0;
	queryrate.onemin = queryrate.fivemin = queryrate.fifteenmin = 0;
	addrate.onemin = addrate.fivemin = addrate.fifteenmin = 0;
	delrate.onemin = delrate.fivemin = delrate.fifteenmin = 0;
	updrate.onemin = updrate.fivemin = updrate.fifteenmin = 0;
	memdbsize = 0;

	log_info("memdb interface initialised.");
	return 0;
}

void
memdb_delete()
{
	int i;

	if (memdb == NULL)
		return;

	for (i = 0; i < HASHSIZE; i++) {
		cachent_t *p;

		p = memdb[i];
		while (p != NULL) {
			cachent_t *q;

			q = p->next;
			ent_delete(&p->ent);
			free(p);
			p = q;
		}
	}
	free(memdb);
	memdb = NULL;
}

int
memdb_add(const unsigned char *key, int keylen, array_t *vals)
{
	cachent_t *ent;
	int hkey;
	time_t curtime;

	ent = malloc(sizeof(cachent_t));
	if (ent == NULL) {
		log_err("Couldn't create memdb entry for `%s': %s.",
			key, strerror(errno));
		return -1;
	}
	ent->next = NULL;
	ent->ent.key = NULL;
	ent->ent.type = C_UPD;
	ent->ent.typeval = NULL;
	ent->ent.upd_time = NULL;

	ent->ent.key = malloc((keylen + 1) * sizeof(char));
	if (ent->ent.key == NULL) {
		log_err("Couldn't create entry key for `%s': %s.",
			key, strerror(errno));
		ent_delete(&ent->ent);
		return -1;
	}
	memcpy(ent->ent.key, key, keylen);
	ent->ent.keylen = keylen;

	ent->ent.vals = array_new();
	if (ent->ent.vals == NULL) {
		ent_delete(&ent->ent);
		return -1;
	}
	if (array_dup(ent->ent.vals, vals) == -1) {
		ent_delete(&ent->ent);
		return -1;
	}

	ent->ent.upd_time = malloc(sizeof(time_t));
	if (ent->ent.upd_time == NULL) {
		ent_delete(&ent->ent);
		return -1;
	}
	curtime = time(NULL);
	memcpy(ent->ent.upd_time, &curtime, sizeof(time_t));

	/* Cache non-results for a minute. */
	if (vals->nitems == 0) {
		time_t expire;

		ent->ent.type = C_EXPIRE;
		ent->ent.typeval = malloc(sizeof(time_t));
		expire = time(NULL);
		expire += config.null_cache_timeout;
		memcpy(ent->ent.typeval, &expire, sizeof(time_t));
	}

	hkey = hashkey(key, keylen);
	rw_mutex_write_lock(memdb_lk);
	if (memdb[hkey] == NULL) {
		memdb[hkey] = ent;
		memdbsize++;
	} else {
		cachent_t *p, *q;

		p = memdb[hkey];
		q = NULL;
		while (p->next != NULL &&
		       !(keylen == p->ent.keylen &&
		       memcmp(p->ent.key, key, p->ent.keylen) == 0)) {
			q = p;
			p = p->next;
		}
		if (keylen == p->ent.keylen &&
		    memcmp(p->ent.key, key, keylen) == 0) {
			/* Already in memdb. */

			log_info("(add) %s already exists.", key);

			switch (p->ent.type) {
			case C_EXPIRE:
			case C_DEL:
				/*
				 * Deletable entries. Trash 'em and
				 * add 'em.
				 */

				if (q == NULL)
					memdb[hkey] = ent;
				else
					q->next = ent;
				ent->next = p->next;

				ent_delete(&p->ent);
				free(p);
				break;

			case C_UPD:
			case C_TRYVAL:
			default:
				/* XXX: Do an update instead of an add. */
				log_info("XXX: Can't handle request.");
			}
		} else {
			p->next = ent;
			memdbsize++;
		}
	}

	add_sec++;
	rw_mutex_unlock(memdb_lk);
	return 0;
}

int
memdb_del(const unsigned char *key, int keylen)
{
	cachent_t *p;
	int hkey;

	hkey = hashkey(key, keylen);

	rw_mutex_read_lock(memdb_lk);
	p = memdb[hkey];
	while (p != NULL &&
	       (keylen != p->ent.keylen || memcmp(p->ent.key, key, keylen)))
		p = p->next;

	if (p == NULL) {
		array_t *aa;

		/*
		 * Entry not found. We have to add it ourselves.
		 */
		rw_mutex_unlock(memdb_lk);

		aa = array_new();
		if (aa == NULL)
			return -1;
		if (memdb_add(key, keylen, aa) == -1)
			return -1;

		/*
		 * The entry has been added - now go back and mark it
		 * for deletion.
		 */
		return memdb_del(key, keylen);
	}

	rw_mutex_write_lock(memdb_lk);
	if (p->ent.typeval != NULL) {
		free(p->ent.typeval);
		p->ent.typeval = NULL;
	}

	p->ent.type = C_DEL;
	if (p->ent.vals != NULL)
		array_delete(p->ent.vals);

	del_sec++;
	rw_mutex_unlock(memdb_lk);

	return 0;
}

int
memdb_get(const unsigned char *key, int keylen, array_t *vals)
{
	cachent_t *p, *q;
	int hkey;

	hkey = hashkey(key, keylen);
	rw_mutex_read_lock(memdb_lk);
	p = memdb[hkey];
	q = NULL;
	while (p != NULL &&
	       (keylen != p->ent.keylen || memcmp(p->ent.key, key, keylen))) {
		q = p;
		p = p->next;
	}

	if (p == NULL) {
		rw_mutex_unlock(memdb_lk);
		return -1;
	}

	switch (p->ent.type) {
	case C_DEL:
		rw_mutex_unlock(memdb_lk);
		query_sec++;
		return 0;
	case C_EXPIRE:
		if (p->ent.typeval != NULL) {
			time_t now, expire;

			now = time(NULL);
			expire = *((time_t *)p->ent.typeval);
			if (now >= expire) {
				ent_delete(&p->ent);
				if (q != NULL)
					q->next = p->next;
				else
					memdb[hkey] = p->next;
				free(p);
				rw_mutex_unlock(memdb_lk);
				log_info("DEBUG: (get) %s has expired", key);
				return -1;
			}
		}
		break;
	default:
		if (p->ent.upd_time != NULL) {
			time_t upd_time;

			upd_time = *p->ent.upd_time;
			if (cdb_mtime > upd_time) {
				ent_delete(&p->ent);
				if (q != NULL)
					q->next = p->next;
				else
					memdb[hkey] = p->next;
				free(p);
				rw_mutex_unlock(memdb_lk);
				log_info("DEBUG: (get) %s is out of date", key);
				return -1;
			}
		}
		break;
	}

	if (array_dup(vals, p->ent.vals) == -1) {
		rw_mutex_unlock(memdb_lk);
		return -1;
	}

	rw_mutex_unlock(memdb_lk);
	query_sec++;
	return 0;
}

int
memdb_upd(const unsigned char *key, int keylen, array_t *vals)
{
	cachent_t *p;
	int hkey;
	time_t curtime;

	/* First make sure that we have enough values. */
	if (vals->nitems != db_fieldcount) {
		log_err("Update tried with wrong value count.");
		return -1;
	}

	hkey = hashkey(key, keylen);
	rw_mutex_read_lock(memdb_lk);
	p = memdb[hkey];
	while (p != NULL &&
	       (keylen != p->ent.keylen || memcmp(p->ent.key, key, keylen)))
		p = p->next;

	if (p == NULL) {
		/* Entry not found. Add it ourselves. */
		rw_mutex_unlock(memdb_lk);
		return memdb_add(key, keylen, vals);
	}
	rw_mutex_unlock(memdb_lk);

	/* Entry found, lets replace it with our copy. */

	rw_mutex_write_lock(memdb_lk);
	p->ent.type = C_UPD;
	p->ent.typeval = NULL;
	curtime = time(NULL);
	memcpy(p->ent.upd_time, &curtime, sizeof(time_t));
	array_delete(p->ent.vals);

	p->ent.vals = array_new();
	if (p->ent.vals == NULL) {
		rw_mutex_unlock(memdb_lk);
		return -1;
	}

	if (array_dup(p->ent.vals, vals) == -1) {
		rw_mutex_unlock(memdb_lk);
		return -1;
	}

	upd_sec++;
	rw_mutex_unlock(memdb_lk);

	return 0;
}

int
memdb_stats(array_t *statarr)
{
	char buffer[512];

	/* Convert q/m, q/5m, and q/15m into q/s. */
	snprintf(buffer, sizeof(buffer), "MEMDB get: %.2f, %.2f, %.2f",
		 queryrate.onemin, queryrate.fivemin, queryrate.fifteenmin);
	if (array_add(statarr, strlen(buffer), buffer, ARRTERM) == -1)
		return -1;

	snprintf(buffer, sizeof(buffer), "MEMDB add: %.2f, %.2f, %.2f",
		 addrate.onemin, addrate.fivemin, addrate.fifteenmin);
	if (array_add(statarr, strlen(buffer), buffer, ARRTERM) == -1)
		return -1;

	snprintf(buffer, sizeof(buffer), "MEMDB del: %.2f, %.2f, %.2f",
		 delrate.onemin, delrate.fivemin, delrate.fifteenmin);
	if (array_add(statarr, strlen(buffer), buffer, ARRTERM) == -1)
		return -1;

	snprintf(buffer, sizeof(buffer), "MEMDB upd: %.2f, %.2f, %.2f",
		 updrate.onemin, updrate.fivemin, updrate.fifteenmin);
	if (array_add(statarr, strlen(buffer), buffer, ARRTERM) == -1)
		return -1;

	snprintf(buffer, sizeof(buffer), "MEMDB total entries: %d", memdbsize);
	return array_add(statarr, strlen(buffer), buffer, ARRTERM);
}

void
memdb_collate()
{
	queryrate.onemin = ((queryrate.onemin * 59) + query_sec) / 60;
	queryrate.fivemin = ((queryrate.fivemin * 299) + query_sec) / 300;
	queryrate.fifteenmin = ((queryrate.fifteenmin * 899) + query_sec) / 900;
	query_sec = 0;

	addrate.onemin = ((addrate.onemin * 59) + add_sec) / 60;
	addrate.fivemin = ((addrate.fivemin * 299) + add_sec) / 300;
	addrate.fifteenmin = ((addrate.fifteenmin * 899) + add_sec) / 900;
	add_sec = 0;

	delrate.onemin = ((delrate.onemin * 59) + del_sec) / 60;
	delrate.fivemin = ((delrate.fivemin * 299) + del_sec) / 300;
	delrate.fifteenmin = ((delrate.fifteenmin * 899) + del_sec) / 900;
	del_sec = 0;

	updrate.onemin = ((updrate.onemin * 59) + upd_sec) / 60;
	updrate.fivemin = ((updrate.fivemin * 299) + upd_sec) / 300;
	updrate.fifteenmin = ((updrate.fifteenmin * 899) + upd_sec) / 900;
	upd_sec = 0;
}
