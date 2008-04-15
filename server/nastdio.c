#include "conf.h"
#include "array.h"
#include "nastd.h"
#include "nastdio.h"
#include "nastipc.h"
#include "cdb.h"
#include "fqm.h"
#include "log.h"
#include "memdb.h"
#include "mysqldb.h"
#include "thread.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

RCSID("$Id: nastdio.c,v 1.13 2001/10/29 11:17:13 shmit Exp $");

extern fqm_t *fqm;

extern fieldent *db_fields;
extern char db_key[1024];
extern short db_fieldcount;

extern time_t start_time;

struct _client_opts {
	nast_options *opts;
	rw_mutex_t *opt_lk;
	thread_t *tid;
	int sock;
};
typedef struct _client_opts client_opts;

static int
arr_send_response(int sock, short reqid, char r_code, array_t *aa)
{
	char *s;
	char buffer[1024];
	short l, i, n_reqid, n_l;
	ssize_t wrote;

	/* Save space for buffer length. */
	l = sizeof(short);

	/* Add request ID. */
        n_reqid = htons(reqid);
	memcpy(buffer+l, &n_reqid, sizeof(n_reqid));
	l += sizeof(reqid);

	/* Add OK or ERR. */
	buffer[l] = r_code;
	l += sizeof(char);

	/*
	 * Build the string to be sent back to the client.
	 */
	for (i = 0; i < aa->nitems; i++) {
		int slen;

		s = aa->items[i]->str;
		slen = aa->items[i]->strlen;
		if (l+slen > sizeof(buffer)) {
			log_err("Buffer isn't big enough for all data.");
			return -1;
		}
		memcpy(buffer+l, s, slen);
		l += slen;

		if (i < aa->nitems-1) {
			buffer[l] = NASTSEP;
			l++;
		}
	}

	/* Fill in buffer length. */
        n_l = htons(l);
	memcpy(buffer, &n_l, sizeof(short));

	wrote = 0;
	while (wrote < l) {
		ssize_t rc;

		rc = write(sock, buffer + wrote, l - wrote);
		if (rc == -1) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			log_err("Couldn't write response: %s.",
				strerror(errno));
			return -1;
		}
		wrote += rc;
	}

	return 0;
}

static int
send_response(int sock, short reqid, char r_code, ...)
{
	array_t *aa;
	int rc;
	va_list ap;

	aa = array_new();
	va_start(ap, r_code);
	if (va_array_add(aa, ap) == -1) {
		va_end(ap);
		array_delete(aa);
		return -1;
	}
	va_end(ap);

	rc = arr_send_response(sock, reqid, r_code, aa);
	array_delete(aa);
	return rc;
}

static int
do_opt_set(int s, short reqid, client_opts *cops, const char *buffer,
	   size_t bufflen)
{
	int i;

	rw_mutex_write_lock(cops->opt_lk);
	for (i = 0; i < bufflen; i += 2) {
		switch (buffer[i]) {
		case OPTQCACHE:
			if (buffer[i+1] == OPTFALSE)
				cops->opts->use_qcache = NASTFALSE;
			else
				cops->opts->use_qcache = NASTTRUE;
			break;
		case OPTLOCALDB:
			if (buffer[i+1] == OPTFALSE)
				cops->opts->use_localdb = NASTFALSE;
			else
				cops->opts->use_localdb = NASTTRUE;
			break;
		case OPTFALLASYNC:
			if (buffer[i+1] == OPTFALSE)
				cops->opts->fallthrough_async = NASTFALSE;
			else
				cops->opts->fallthrough_async = NASTTRUE;
			break;
		case OPTALWAYSFALL:
			if (buffer[i+1] == OPTFALSE)
				cops->opts->always_fallthrough = NASTFALSE;
			else
				cops->opts->always_fallthrough = NASTTRUE;
			break;
		case OPTFAILONCE:
			if (buffer[i+1] == OPTFALSE)
				cops->opts->fail_once = NASTFALSE;
			else
				cops->opts->fail_once = NASTTRUE;
			break;
		case OPTNOFALLTHROUGH:
			if (buffer[i+1] == OPTFALSE)
				cops->opts->no_fallthrough = NASTFALSE;
			else
				cops->opts->no_fallthrough = NASTTRUE;
			break;
		default:
			rw_mutex_unlock(cops->opt_lk);
			log_err("Unknown option: `0x%x'.\n", buffer[i]);
			(void)send_response(s, reqid, NASTERR,
					    strlen("Sent unknown option"),
					    "Sent unknown option", ARRTERM);
			return -1;
		}
	}
	rw_mutex_unlock(cops->opt_lk);

	if (send_response(s, reqid, NASTOK, ARRTERM) == -1)
		return -1;
	return 0;
}

static int
do_opt_get(int s, short reqid, client_opts *cops)
{
	char buffer[512];
	int bufflen;

	bufflen = 0;
	rw_mutex_read_lock(cops->opt_lk);

	buffer[bufflen] = OPTQCACHE;
	if (cops->opts->use_qcache)
		buffer[bufflen+1] = OPTTRUE;
	else
		buffer[bufflen+1] = OPTFALSE;
	bufflen += 2;

	buffer[bufflen] = OPTLOCALDB;
	if (cops->opts->use_localdb)
		buffer[bufflen+1] = OPTTRUE;
	else
		buffer[bufflen+1] = OPTFALSE;
	bufflen += 2;

	buffer[bufflen] = OPTFALLASYNC;
	if (cops->opts->fallthrough_async)
		buffer[bufflen+1] = OPTTRUE;
	else
		buffer[bufflen+1] = OPTFALSE;
	bufflen += 2;

	buffer[bufflen] = OPTALWAYSFALL;
	if (cops->opts->always_fallthrough)
		buffer[bufflen+1] = OPTTRUE;
	else
		buffer[bufflen+1] = OPTFALSE;
	bufflen += 2;

	buffer[bufflen] = OPTFAILONCE;
	if (cops->opts->fail_once)
		buffer[bufflen+1] = OPTTRUE;
	else
		buffer[bufflen+1] = OPTFALSE;
	bufflen += 2;

	buffer[bufflen] = OPTNOFALLTHROUGH;
	if (cops->opts->no_fallthrough)
		buffer[bufflen+1] = OPTTRUE;
	else
		buffer[bufflen+1] = OPTFALSE;
	bufflen += 2;

	buffer[bufflen] = '\0';

	rw_mutex_unlock(cops->opt_lk);

	if (send_response(s, reqid, NASTOK, bufflen, buffer, ARRTERM) == -1)
		return -1;
	return 0;
}

static
REQHANDLER(do_fallthrough)
{
	array_t *aa;
	int rc;

	aa = array_new();
	if (aa == NULL) {
		log_info("Couldn't allocate array: %s.", strerror(errno));
		(void)send_response(req->sock, req->reqid, NASTERR,
				    strlen("Couldn't allocate return buffer"),
				    "Couldn't allocate return buffer", ARRTERM);
		return -1;
	}

	rc = mysqldb_get(self, req->req, strlen(req->req), aa);
	if (rc == -1) {
		log_info("Couldn't find %s in MySQL DB.", req->req);
		rc = send_response(req->sock, req->reqid, NASTOK, ARRTERM);
		array_delete(aa);
		return -1;
	}

	if (arr_send_response(req->sock, req->reqid, NASTOK, aa) == -1) {
		array_delete(aa);
		return -1;
	}

	/* If we're this far, the memdb doesn't have this key. Add it. */
	(void)memdb_add((unsigned char *)req->req, strlen(req->req), aa);

	array_delete(aa);

	return 0;
}

static int
do_get(int s, short reqid, client_opts *cops, const char *buffer,
       size_t bufflen)
{
	int fallthrough_flag;
	array_t *aa;

	aa = array_new();
	if (aa == NULL) {
		log_info("Couldn't allocate array: %s.", strerror(errno));
		(void)send_response(s, reqid, NASTERR,
				    strlen("Couldn't allocate return buffer"),
				    "Couldn't allocate return buffer", ARRTERM);
		return -1;
	}

	/* Check memdb first. */
	if (cops->opts->use_qcache && !cops->opts->always_fallthrough) {
		if (memdb_get((unsigned char *)buffer, bufflen, aa) == 0) {
			log_info("Found `%s' in memdb.", buffer);
			if (arr_send_response(s, reqid, NASTOK, aa) == -1) {
				array_delete(aa);
				return -1;
			}
			array_delete(aa);
			return 0;
		}
	}

	/* Just do a CDB lookup for now. */
	if (!cops->opts->always_fallthrough && cops->opts->use_localdb) {
		fallthrough_flag = cdb_get(buffer, bufflen, aa);
		if (fallthrough_flag == -1)
			log_info("Couldn't find `%s' in CDB file.", buffer);
	} else {
		fallthrough_flag = -1;
	}

	/*
	 * If fallthrough_flag is -1, then the CDB query failed, so
	 * we should check fallthrough.
	 */
	if (fallthrough_flag == -1 && !cops->opts->no_fallthrough) {
		request_t *req;

		array_delete(aa);

		log_info("Checking fallthrough for `%s'.", buffer);
		req = req_new(s, reqid, do_fallthrough,
			      buffer, bufflen);
		if (req == NULL) {
			log_err("Couldn't build request for FQM.");
			send_response(s, reqid, NASTERR,
				      strlen("Couldn't build FQM req"),
				      "Couldn't build FQM req", ARRTERM);
			return -1;
		}

		if (fqm_push(fqm, req) == -1) {
			send_response(s, reqid, NASTERR,
				      strlen("Couldn't add req to FQM"),
				      "Couldn't add req to FQM", ARRTERM);
			req_delete(req);
			return -1;
		}
		return 0;
	}

	if (arr_send_response(s, reqid, NASTOK, aa) == -1) {
		array_delete(aa);
		return -1;
	}
	array_delete(aa);

	return 0;
}

static array_t *
build_array(const char *buff, size_t bufflen)
{
	array_t *aa;
	const char *ep, *sp;

	aa = array_new();
	if (aa == NULL) {
		log_err("Couldn't allocate array for input buffer.");
		return NULL;
	}

	sp = buff;
	for (ep = sp; ep <= buff+bufflen; ep++) {
		if ((ep == buff+bufflen || *ep == NASTSEP) &&
		    ep - sp > 0) {
			if (array_add(aa, ep-sp, sp, ARRTERM) == -1) {
				log_err("Couldn't add item to input array.");
				array_delete(aa);
				return NULL;
			}
			sp = ep+1;
		}
	}

	return aa;
}

static int
do_update(int s, short reqid, client_opts *cops, const char *buffer,
	  size_t bufflen)
{
	array_t *aa;
	unsigned char *key;
	int keylen;

	for (keylen = 0; keylen < bufflen; keylen++) {
		if (buffer[keylen] == NASTSEP)
			break;
	}

	if (keylen == bufflen) {
		/* Request only contains key. That's bad. */
		return send_response(s, reqid, NASTERR,
				     strlen("Need values for update"),
				     "Need values for update", ARRTERM);
	}

	key = malloc(sizeof(char) * keylen + 1);
	if (key == NULL) {
		log_err("Couldn't allocate key for update: %s.",
			strerror(errno));
		send_response(s, reqid, NASTERR,
			      strlen("Couldn't allocate key"),
			      "Couldn't allocate key", ARRTERM);
		return -1;
	}
	memcpy(key, buffer, keylen);
	key[keylen + 1] = '\0';

	aa = build_array(buffer+keylen + 1, bufflen-keylen - 1);
	if (aa == NULL) {
		send_response(s, reqid, NASTERR,
			      strlen("Couldn't build your input array"),
			      "Couldn't build your input array", ARRTERM);
		return -1;
	}

	if (memdb_upd(key, keylen, aa) == -1) {
		array_delete(aa);
		return send_response(s, reqid, NASTERR,
				     strlen("Couldn't update cache"),
				     "Couldn't update cache", ARRTERM);
	}
	array_delete(aa);

	return send_response(s, reqid, NASTOK, ARRTERM);
}

static int
do_stats(int s, int reqid)
{
	array_t *aa;
	char buffer[512];
	int up_day, up_hour, up_min, up_sec;
	time_t uptime;

	aa = array_new();

	/* Gather stats from the various databases. */
	if (mysqldb_stats(aa) == -1) {
		send_response(s, reqid, NASTERR,
			      strlen("Couldn't get stats from MySQL"),
			      "Couldn't get stats from MySQL", ARRTERM);
		array_delete(aa);
		return -1;
	}

	if (cdb_stats(aa) == -1) {
		send_response(s, reqid, NASTERR,
			      strlen("Couldn't get stats from cdb"),
			      "Couldn't get stats from cdb", ARRTERM);
		array_delete(aa);
		return -1;
	}

	if (memdb_stats(aa) == -1) {
		send_response(s, reqid, NASTERR,
			      strlen("Couldn't get stats from memdb"),
			      "Couldn't get stats from memdb", ARRTERM);
		array_delete(aa);
		return -1;
	}

	uptime = time(NULL) - start_time;
	up_day = uptime / 86400; uptime -= up_day * 86400;
	up_hour = uptime / 3600; uptime -= up_hour * 3600;
	up_min = uptime / 60; uptime -= up_min * 60;
	up_sec = uptime;

	snprintf(buffer, sizeof(buffer),
		 "Uptime: %d day(s), %02d:%02d:%02d",
		 up_day, up_hour, up_min, up_sec);
	if (array_add(aa, strlen(buffer), buffer, ARRTERM) == -1) {
		send_response(s, reqid, NASTERR,
			      strlen("Couldn't return uptime"),
			      "Couldn't return uptime", ARRTERM);
		array_delete(aa);
		return -1;
	}

	snprintf(buffer, sizeof(buffer),
		 "Version: %s", VERSION);
	if (array_add(aa, strlen(buffer), buffer, ARRTERM) == -1) {
		send_response(s, reqid, NASTERR,
			      strlen("Couldn't return version"),
			      "Couldn't return version", ARRTERM);
		array_delete(aa);
		return -1;
	}

	/* Now send 'em off. */
	if (arr_send_response(s, reqid, NASTOK, aa) == -1) {
		array_delete(aa);
		return -1;
	}
	array_delete(aa);
	return 0;
}

static int
process_cmd(int s, client_opts *cops, const char *buffer, size_t bufflen)
{
	const char *p;
	int l;
	short len, reqid;

	/*
	 * bufflen must be at least six bytes or we have a screwed up
	 * command.
	 */
	if (bufflen < sizeof(len) + sizeof(reqid) + 2*sizeof(char)) {
		log_err("Command sent is too short.");
		return -1;
	}

	for (p = buffer; p < buffer+bufflen; p += len) {
		/* All requests start with a length. */
		memcpy(&len, p, sizeof(reqid));
		len = ntohs(len);
		l = sizeof(len);

		/* Follow with a request ID. */
		memcpy(&reqid, p+l, sizeof(reqid));
		reqid = ntohs(reqid);
		l += sizeof(reqid);

		/* Then have NASTCMD. Make sure it's there. */
		if (p[l] != NASTCMD) {
			log_err("Command doesn't start with NASTCMD.");
			return -1;
		}
		l += sizeof(char);

		/* The next byte says what kind of command it is. */
		switch (p[l++]) {
		case NASTDIE:
			return 1;
			break;
		case NASTOPTSET:
			if (do_opt_set(s, reqid, cops, p+l, len-l) == -1)
				return -1;
			break;
		case NASTOPTGET:
			if (do_opt_get(s, reqid, cops) == -1)
				return -1;
			break;
		case NASTGET:
			if (do_get(s, reqid, cops, p+l, len-l) == -1)
				return -1;
			break;
		case NASTADD:
			log_err("Add command not supported yet.");
			break;
		case NASTDEL:
			log_err("Delete command not supported yet.");
			break;
		case NASTUPD:
			if (do_update(s, reqid, cops, p+l, len-l) == -1)
				return -1;
			break;
		case NASTSTATS:
			if (do_stats(s, reqid) == -1)
				return -1;
			break;
		default:
			return -1;
		}
	}

	return 0;
}

static void
free_cops(client_opts *cops)
{
	if (cops == NULL)
		return;

	close(cops->sock);
	if (cops->opts != NULL)
		free(cops->opts);
	if (cops->opt_lk != NULL)
		free(cops->opt_lk);
	if (cops->tid != NULL)
		free(cops->tid);
	free(cops);
}

static void *
io_looper(void *arg)
{
	client_opts *cops;

	(void)pthread_detach(pthread_self());
	cops = (client_opts *)arg;

	for (;;) {
		ssize_t nbytes;
		char buffer[1024];

		nbytes = recv(cops->sock, buffer, sizeof(buffer), 0);
		if (nbytes == -1) {
			log_err("Couldn't read from socket: %s.",
				strerror(errno));
			break;
		}
		if (nbytes == 0) {
			/* Connection has been closed. Terminate. */
			log_info("Connection closed.");
			break;
		}
		buffer[nbytes] = '\0';

		/* Do command processing on the buffer. */
		if (process_cmd(cops->sock, cops, buffer, nbytes) == 1)
			break;
	}

	free_cops(cops);
	return NULL;
}

static void
set_default_opts(nast_options *opts)
{
	opts->use_qcache = NASTTRUE;
	opts->use_localdb = NASTTRUE;
	opts->fallthrough_async = NASTFALSE;
	opts->always_fallthrough = NASTFALSE;
	opts->fail_once = NASTFALSE;
	opts->no_fallthrough = NASTFALSE;
}

int
io_new(int s)
{
	client_opts *cops;

	log_info("Connection opened.");

	cops = malloc(sizeof(client_opts));
	if (cops == NULL) {
		log_err("Couldn't allocate client option structure: %s.",
			strerror(errno));
		return -1;
	}

	/* Pre-set these. free_cops() relies on them being set. */
	cops->opts = NULL;
	cops->opt_lk = NULL;
	cops->sock = s;
	cops->tid = NULL;

	cops->opts = malloc(sizeof(nast_options));
	if (cops->opts == NULL) {
		log_err("Couldn't allocate client options structure: %s.",
			strerror(errno));
		free_cops(cops);
		return -1;
	}
	set_default_opts(cops->opts);

	cops->opt_lk = malloc(sizeof(rw_mutex_t));
	if (cops->opt_lk == NULL) {
		log_err("Couldn't allocate client options mutex: %s.",
			strerror(errno));
		free_cops(cops);
		return -1;
	}

	if (rw_mutex_new(cops->opt_lk)) {
		log_err("Couldn't initialise client options mutex: %s.",
			strerror(errno));
		free_cops(cops);
		return -1;
	}

	cops->tid = malloc(sizeof(thread_t));
	if (cops->tid == NULL) {
		log_err("Couldn't allocate initial client thread: %s.",
			strerror(errno));
		free_cops(cops);
		return -1;
	}

	if (thread_new(cops->tid, io_looper, cops)) {
		log_err("Couldn't start initial looper thread: %s.",
			strerror(errno));
		free_cops(cops);
		return -1;
	}

	return 0;
}
