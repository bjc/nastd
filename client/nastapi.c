#include "conf.h"
#include "nastd.h"
#include "nastipc.h"
#include "thread.h"

#include <errno.h>
#include <thread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <signal.h>
#include <unistd.h>

RCSID("$Id: nastapi.c,v 1.13 2001/10/29 11:17:11 shmit Exp $");

char *nast_errmsgs[] = {
	"No errors",
	"The NASTD server has gone away",
	"Couldn't allocate enough memory",
	"Server response is unknown",
	"Connection to server timed out",
	"Unknown option returned",
	NULL
};

static nast_string_t *
nast_string_new(int slen, const char *data)
{
	nast_string_t *tmp;

	tmp = malloc(sizeof(nast_string_t));
	if (tmp == NULL)
		return NULL;

	tmp->strdata = malloc((slen+1) * sizeof(char *));
	if (tmp == NULL)
		return NULL;
	memcpy(tmp->strdata, data, slen);
	tmp->strdata[slen] = '\0';
	tmp->strlen = slen;

	return tmp;
}

static void
nast_string_delete(nast_string_t *string)
{
	string->strlen = 0;
	free(string->strdata);
	string->strdata = NULL;
	free(string);
}

nast_array *
nast_array_new()
{
	nast_array *aa;

	aa = malloc(sizeof(nast_array));
	if (aa == NULL)
		return NULL;

	aa->nitems = 0;
	aa->items = NULL;
	return aa;
}

int
nast_array_add(nast_array *array, short len, const char *data)
{
	const int GRANULARITY = 10;

	if (array->nitems % GRANULARITY == 0) {
		nast_string_t **tmp;
		tmp = realloc(array->items, sizeof(char *) *
			      (GRANULARITY + array->nitems));
		if (tmp == NULL)
			return -1;
		array->items = tmp;
	}
	array->nitems++;

	array->items[array->nitems-1] = nast_string_new(len, data);
	if (array->items[array->nitems-1] == NULL)
		return -1;

	return 0;
}

void
nast_array_delete(nast_array *array)
{
	nast_free_result(array);
}

static nast_response *
response_new()
{
	nast_response *tmp;

	tmp = malloc(sizeof(nast_response));
	if (tmp == NULL)
		return NULL;

	tmp->buffer = NULL;
	tmp->bufflen = 0;
	tmp->errcode = NAST_OK;
	tmp->errmsg = NULL;
	tmp->reqid = -1;

	return tmp;
}

static nast_response *
getmyresponse(nasth *s, unsigned short reqid)
{
	if (s->nthreads < reqid)
		return NULL;

	return s->responses[reqid-1];
}

static void
nast_set_error(nasth *s, unsigned short reqid, errcodes code)
{
	nast_response *ar;

	ar = getmyresponse(s, reqid);
	if (ar == NULL)
		return;

	ar->errcode = code;
}

static int
grow_responses(nasth *s, unsigned short maxitems)
{
	nast_response **tmp_resp;
	int i;

	if (s->nthreads >= maxitems)
		return 0;

	tmp_resp = realloc(s->responses, maxitems * sizeof(nast_response *));
	if (tmp_resp == NULL)
		return -1;

	s->responses = tmp_resp;
	for (i = s->nthreads; i < maxitems; i++) {
		s->responses[i] = response_new();
		if (s->responses[i] == NULL)
			return -1;
	}

	s->nthreads = maxitems;
	return 0;
}

static nast_array *
build_result(nasth *s, const char *buff, short bufflen)
{
	nast_array *aa;
	const char *s_p, *e_p;
	short l;

	aa = nast_array_new();
	if (aa == NULL) {
		nast_set_error(s, thread_id(), NAST_NOMEM);
		return NULL;
	}

	/* Parse the buff into an array. */
	l = 0;
	aa->nitems = 0;
	aa->items = NULL;
	s_p = buff+l;
	for (e_p = s_p; e_p <= buff+bufflen; e_p++) {
		if (e_p == buff+bufflen || *e_p == NASTSEP) {
			if (nast_array_add(aa, e_p - s_p, s_p) == -1) {
				nast_set_error(s, thread_id(), NAST_NOMEM);
				return NULL;
			}
			s_p = e_p + 1;
		}
	}

	return aa;
}

static int
addresponse(nasth *s, unsigned short reqid, char *buffer, short len)
{
	nast_response *ar;

	switch (buffer[0]) {
	case NASTOK:
		nast_set_error(s, reqid, NAST_OK);
		break;
	case NASTERR:
		nast_set_error(s, reqid, NAST_SERVER_ERR);
		break;
	default:
		nast_set_error(s, reqid, NAST_UNKNOWN_RESPONSE);
	}

	ar = getmyresponse(s, reqid);
	if (ar == NULL) {
		/*
		 * Somehow we got a response for something we didn't
		 * request.
		 */
		return -1;
	}

	ar->reqid = reqid;
	ar->buffer = malloc(len-1);
	if (ar->buffer == NULL) {
		nast_set_error(s, reqid, NAST_NOMEM);
		return -1;
	}
	memcpy(ar->buffer, buffer+1, len-1);
	ar->bufflen = len-1;
	return 0;
}

static void
delresponse(nasth *s, unsigned short reqid)
{
	nast_response *ar;

	ar = getmyresponse(s, reqid);
	if (ar == NULL)
		return;

	ar->reqid = -1;
	if (ar->buffer)
		free(ar->buffer);
	ar->bufflen = 0;
}

static int
checkresponse(nasth *s, unsigned short reqid)
{
	nast_response *ar;

	if (reqid > s->nthreads)
		return -1;

	ar = getmyresponse(s, reqid);
	if (ar == NULL || (short)ar->reqid == -1)
		return -1;

	return 0;
}

static int
sendcmd(nasth *s, const char *buff, short len)
{
	ssize_t wrote;

	/* Make sure there's room in the response array. */
	if (grow_responses(s, thread_id()) == -1) {
		/*
		 * XXX: This is a fatal error.
		 * Find some better way to handle this.
		 */
		exit(1);
	}

	/* If we're sending a command, make sure we get rid of old data. */
	delresponse(s, thread_id());

	/* Send the command. */
	wrote = 0;
	while (wrote < len) {
		ssize_t rc;

		rc = write(s->socket, buff + wrote, len - wrote);
		if (rc == -1) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			nast_set_error(s, thread_id(), NAST_SERVER_GONE);
			return -1;
		}
		wrote += rc;
	}

	/* Check status. */
	return 0;
}

static int
getresponse(nasth *s)
{
	char buffer[1024];
	nast_response *ar;
	int ntries, rc;
	unsigned short tid;
	short nbytes;

	ntries = 0;
	tid = thread_id();
reread:
	_nast_mutex_lock(s->lock);

	/*
	 * See if we already have the response from another thread which
	 * may have found it first.
	 */
	rc = checkresponse(s, tid);
	if (rc == 0) {
		_nast_mutex_unlock(s->lock);
	} else {
		/* Not in the already read stuff, check the network. */
		char *p;
		short bufflen;
		unsigned short reqid;
		fd_set readfds;
		struct timeval timeout;

		FD_ZERO(&readfds);
		FD_SET(s->socket, &readfds);
		timeout.tv_sec = 0;
		timeout.tv_usec = 10000;
		switch(select(s->socket+1, &readfds, NULL, NULL, &timeout)) {
		case -1:
			_nast_mutex_unlock(s->lock);
			nast_set_error(s, tid, NAST_SERVER_GONE);
			return -1;
		case 0:
			/* Timeout expired. */
			_nast_mutex_unlock(s->lock);
			if (ntries++ < 50)
				goto reread;
			nast_set_error(s, tid, NAST_TIMEDOUT);
			return -1;
		}
		nbytes = recv(s->socket, buffer, sizeof(buffer), 0);
		if (nbytes == -1) {
			_nast_mutex_unlock(s->lock);
			nast_set_error(s, tid, NAST_SERVER_GONE);
			return -1;
		} else if (nbytes == 0) {
			/* No response from server. That's bad. Terminate. */
			_nast_mutex_unlock(s->lock);
			nast_set_error(s, tid, NAST_SERVER_GONE);
			return -1;
		} else if (nbytes <
			   sizeof(bufflen) + sizeof(reqid) + sizeof(char)) {
			/* Response is too short. */
			_nast_mutex_unlock(s->lock);
			nast_set_error(s, tid, NAST_UNKNOWN_RESPONSE);
			return -1;
		}

		for (p = buffer; p < buffer+nbytes; p += bufflen) {
			int l;

			memcpy(&bufflen, p, sizeof(bufflen));
			bufflen = ntohs(bufflen);
			l = sizeof(bufflen);

			/* Sanity check, just in case data gets munged. */
			if (bufflen <= 0 || bufflen > nbytes) {
				_nast_mutex_unlock(s->lock);
				nast_set_error(s, tid, NAST_UNKNOWN_RESPONSE);
				return -1;
			}

			memcpy(&reqid, p+l, sizeof(reqid));
			reqid = ntohs(reqid);
			l += sizeof(reqid);

			/* Save this response on the response array. */
			addresponse(s, reqid, p+l, bufflen-l);
		}
		/* Check the response array to see if we got our response. */
		_nast_mutex_unlock(s->lock);
		goto reread;
	}

	ar = getmyresponse(s, tid);
	if (ar->errcode == NAST_OK)
		return 0;
	else
		return -1;
}

nasth *
nast_sphincter_new(const char *sock_path)
{
	nasth *tmp_h;
	struct sockaddr_un sunix;
	int rc;

	tmp_h = malloc(sizeof(nasth));
	if (tmp_h == NULL) {
		fprintf(stderr,
			"ERROR: Couldn't make space for sphincter: %s.\n",
			strerror(errno));
		return NULL;
	}

	tmp_h->nthreads = 0;
	tmp_h->responses = NULL;

	tmp_h->lock = malloc(sizeof(_nast_mutex_t));
	if (tmp_h->lock == NULL) {
		fprintf(stderr,
			"ERROR: Couldn't create NAST lock: %s.\n",
			strerror(errno));
		nast_sphincter_close(tmp_h);
		return NULL;
	}
	rc = _nast_mutex_new(tmp_h->lock);
	if (rc) {
		fprintf(stderr,
			"ERROR: Couldn't initialise NAST lock: %s.\n",
			strerror(rc));
		nast_sphincter_close(tmp_h);
		return NULL;
	}

	tmp_h->socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (tmp_h->socket == -1) {
		fprintf(stderr,
			"ERROR: Couldn't initialise socket: %s.\n",
			strerror(errno));
		nast_sphincter_close(tmp_h);
		return NULL;
	}

	memset(&sunix, 0, sizeof(sunix));
	if (sock_path == NULL)
		snprintf(sunix.sun_path, sizeof(sunix.sun_path), NASTHOLE);
	else
		strncpy(sunix.sun_path, sock_path, sizeof(sunix.sun_path));
	sunix.sun_family = AF_UNIX;

	if (connect(tmp_h->socket, (struct sockaddr *)&sunix,
		    sizeof(sunix)) == -1) {
		fprintf(stderr,
			"ERROR: Couldn't connect to server: %s.\n",
			strerror(errno));
		nast_sphincter_close(tmp_h);
		return NULL;
	}
	return tmp_h;
}

void
nast_sphincter_close(nasth *s)
{
	int i;

	if (s == NULL)
		return;

	s->nthreads = 0;

	if (s->lock != NULL) {
		_nast_mutex_delete(s->lock);
		free(s->lock);
		s->lock = NULL;
	}

	if (s->socket != -1) {
		close(s->socket);
		s->socket = -1;
	}

	if (s->responses != NULL) {
		for (i = 0; i < s->nthreads; i++) {
			if (s->responses[i]->buffer != NULL)
				free(s->responses[i]->buffer);
			free(s->responses[i]);
			s->responses[i] = NULL;
		}
		free(s->responses);
		s->responses = NULL;
	}

	free(s);
}

nast_array *
nast_get_result(nasth *s)
{
	nast_response *ar;

	ar = getmyresponse(s, thread_id());
	if (ar == NULL)
		return NULL;

	return build_result(s, ar->buffer, ar->bufflen);
}

void
nast_free_result(nast_array *aa)
{
	int i;

	if (aa->items) {
		for (i = 0; i < aa->nitems; i++)
			nast_string_delete(aa->items[i]);
		free(aa->items);
		aa->items = NULL;
	}
	aa->nitems = 0;
	free(aa);
}

static int
add_reqid(char *buffer)
{
	unsigned short tid;

	tid = thread_id();
	memcpy(buffer, &htons(tid), sizeof(tid));

	return sizeof(tid);
}

int
nast_options_get(nasth *s, nast_options *opts)
{
	nast_array *aa;
	char buffer[512];
	short bufflen, i;

	if (s == NULL) {
		fprintf(stderr, "ERROR: Can't get options: no sphincter.\n");
		return -1;
	}

	bufflen = sizeof(short);
	bufflen += add_reqid(buffer+bufflen);

	snprintf(buffer+bufflen, sizeof(buffer)-bufflen, "%c%c",
		 NASTCMD, NASTOPTGET);
	bufflen += 2 * sizeof(char);
	memcpy(buffer, &htons(bufflen), sizeof(short));
	sendcmd(s, buffer, bufflen);
	if (getresponse(s) == -1)
		return -1;

	/* Copy out results and free them. */
	aa = nast_get_result(s);

	if (aa->nitems != 1) {
		nast_set_error(s, thread_id(), NAST_UNKNOWN_RESPONSE);
		return -1;
	}

	if (sizeof(buffer) < aa->items[0]->strlen)
		bufflen = sizeof(buffer);
	else
		bufflen = aa->items[0]->strlen;
	memcpy(buffer, aa->items[0]->strdata, bufflen);
	nast_free_result(aa);

	/* Parse return into options. */
	for (i = 0; i < bufflen; i+=2) {
		switch (buffer[i]) {
		case OPTQCACHE:
			if (buffer[i+1] == OPTFALSE)
				opts->use_qcache = NASTFALSE;
			else
				opts->use_qcache = NASTTRUE;
			break;
		case OPTLOCALDB:
			if (buffer[i+1] == OPTFALSE)
				opts->use_localdb = NASTFALSE;
			else
				opts->use_localdb = NASTTRUE;
			break;
		case OPTFALLASYNC:
			if (buffer[i+1] == OPTFALSE)
				opts->fallthrough_async = NASTFALSE;
			else
				opts->fallthrough_async = NASTTRUE;
			break;
		case OPTALWAYSFALL:
			if (buffer[i+1] == OPTFALSE)
				opts->always_fallthrough = NASTFALSE;
			else
				opts->always_fallthrough = NASTTRUE;
			break;
		case OPTFAILONCE:
			if (buffer[i+1] == OPTFALSE)
				opts->fail_once = NASTFALSE;
			else
				opts->fail_once = NASTTRUE;
			break;
		case OPTNOFALLTHROUGH:
			if (buffer[i+1] == OPTFALSE)
				opts->no_fallthrough = NASTFALSE;
			else
				opts->no_fallthrough = NASTTRUE;
			break;
		default:
			nast_set_error(s, thread_id(), NAST_UNKNOWN_OPT);
			return -1;
		}
	}

	return 0;
}

int
nast_options_set(nasth *s, nast_options *opts)
{
	char buffer[512];
	short bufflen;

	if (s == NULL) {
		fprintf(stderr, "ERROR: Can't set options: no sphincter.\n");
		return -1;
	}

	bufflen = sizeof(short);
	bufflen += add_reqid(buffer+bufflen);

	snprintf(buffer+bufflen, sizeof(buffer)-bufflen, "%c%c",
		 NASTCMD, NASTOPTSET);
	bufflen += 2*sizeof(char);

	buffer[bufflen] = OPTQCACHE;
	if (opts->use_qcache)
		buffer[bufflen+1] = OPTTRUE;
	else
		buffer[bufflen+1] = OPTFALSE;
	bufflen += 2;

	buffer[bufflen] = OPTLOCALDB;
	if (opts->use_localdb)
		buffer[bufflen+1] = OPTTRUE;
	else
		buffer[bufflen+1] = OPTFALSE;
	bufflen += 2;

	buffer[bufflen] = OPTFALLASYNC;
	if (opts->fallthrough_async)
		buffer[bufflen+1] = OPTTRUE;
	else
		buffer[bufflen+1] = OPTFALSE;
	bufflen += 2;

	buffer[bufflen] = OPTALWAYSFALL;
	if (opts->always_fallthrough)
		buffer[bufflen+1] = OPTTRUE;
	else
		buffer[bufflen+1] = OPTFALSE;
	bufflen += 2;

	buffer[bufflen] = OPTFAILONCE;
	if (opts->fail_once)
		buffer[bufflen+1] = OPTTRUE;
	else
		buffer[bufflen+1] = OPTFALSE;
	bufflen += 2;

	buffer[bufflen] = OPTNOFALLTHROUGH;
	if (opts->no_fallthrough)
		buffer[bufflen+1] = OPTTRUE;
	else
		buffer[bufflen+1] = OPTFALSE;
	bufflen += 2;

	memcpy(buffer, &htons(bufflen), sizeof(short));
	sendcmd(s, buffer, bufflen);
	return getresponse(s);
}

int
nast_add(nasth *s, const char *query)
{
	char buffer[512];
	short bufflen;

	if (s == NULL) {
		fprintf(stderr, "ERROR: Can't add: no sphincter.\n");
		return -1;
	}

	bufflen = sizeof(short);
	bufflen += add_reqid(buffer+bufflen);

	snprintf(buffer+bufflen, sizeof(buffer)-bufflen, "%c%c%s",
		 NASTCMD, NASTADD, query);
	bufflen += (2 + strlen(query))*sizeof(char);

	memcpy(buffer, &htons(bufflen), sizeof(short));
	sendcmd(s, buffer, bufflen);
	return getresponse(s);
}

int
nast_del(nasth *s, const char *query)
{
	char buffer[512];
	short bufflen;

	if (s == NULL) {
		fprintf(stderr, "ERROR: Can't delete: no sphincter.\n");
		return -1;
	}

	bufflen = sizeof(short);
	bufflen += add_reqid(buffer+bufflen);

	snprintf(buffer+bufflen, sizeof(buffer)-bufflen, "%c%c%s",
		 NASTCMD, NASTDEL, query);
	bufflen += (2 + strlen(query))*sizeof(char);

	memcpy(buffer, &htons(bufflen), sizeof(short));
	sendcmd(s, buffer, bufflen);
	return getresponse(s);
}

int
nast_get(nasth *s, const char *query)
{
	char buffer[512];
	short bufflen;

	if (s == NULL) {
		fprintf(stderr, "ERROR: Can't get: no sphincter.\n");
		return -1;
	}

	bufflen = sizeof(short);
	bufflen += add_reqid(buffer+bufflen);

	snprintf(buffer+bufflen, sizeof(buffer)-bufflen, "%c%c%s",
		 NASTCMD, NASTGET, query);
	bufflen += (2 + strlen(query))*sizeof(char);

	memcpy(buffer, &htons(bufflen), sizeof(short));
	sendcmd(s, buffer, bufflen);
	return getresponse(s);
}

void
nast_die(nasth *s)
{
	char buffer[512];
	short bufflen;

	if (s == NULL) {
		fprintf(stderr, "ERROR: Can't kill nast: no sphincter.\n");
		return;
	}

	bufflen = sizeof(short);
	bufflen += add_reqid(buffer+bufflen);

	snprintf(buffer+bufflen, sizeof(buffer)-bufflen, "%c%c",
		 NASTCMD, NASTDIE);
	bufflen += 2 * sizeof(char);

	memcpy(buffer, &htons(bufflen), sizeof(short));
	sendcmd(s, buffer, bufflen);
	return;
}

int
nast_upd(nasth *s, const char *key, nast_array *valarray)
{
	char buffer[512];
	int i;
	short bufflen;

	if (s == NULL) {
		fprintf(stderr, "ERROR: Can't update: no sphincter.\n");
		return -1;
	}

	bufflen = sizeof(short);
	bufflen += add_reqid(buffer+bufflen);

	snprintf(buffer+bufflen, sizeof(buffer)-bufflen, "%c%c%s%c",
		 NASTCMD, NASTUPD, key, NASTSEP);
	bufflen += (3 + strlen(key))*sizeof(char);

	for (i = 0; i < valarray->nitems; i++) {
		char *str;
		short slen;

		str = valarray->items[i]->strdata;
		slen = valarray->items[i]->strlen;
		if (bufflen + slen > sizeof(buffer)) {
			nast_set_error(s, thread_id(), NAST_NOMEM);
			return -1;
		}

		memcpy(buffer+bufflen, str, slen);
		bufflen += slen;

		if (i < valarray->nitems - 1) {
			buffer[bufflen] = NASTSEP;
			bufflen += sizeof(char);
		}
	}

	memcpy(buffer, &htons(bufflen), sizeof(short));
	sendcmd(s, buffer, bufflen);
	return getresponse(s);
}

int
nast_stats(nasth *s)
{
	char buffer[512];
	short bufflen;

	if (s == NULL) {
		fprintf(stderr, "ERROR: Can't get stats: no sphincter.\n");
		return -1;
	}

	bufflen = sizeof(short);
	bufflen += add_reqid(buffer+bufflen);

	snprintf(buffer+bufflen, sizeof(buffer)-bufflen, "%c%c",
		 NASTCMD, NASTSTATS);
	bufflen += 2 * sizeof(char);
	memcpy(buffer, &htons(bufflen), sizeof(short));
	sendcmd(s, buffer, bufflen);
	return getresponse(s);
}

errcodes
nast_geterr(nasth *s)
{
	nast_response *ar;

	if (s == NULL)
		return NAST_SERVER_GONE;

	ar = getmyresponse(s, thread_id());
	if (ar == NULL)
		return NAST_OK;

	return ar->errcode;
}

char *
nast_errmsg(nasth *s)
{
	nast_response *ar;
	errcodes ec;

	ec = nast_geterr(s);
	if (ec == NAST_SERVER_ERR) {
		nast_array *aa;

		ar = getmyresponse(s, thread_id());
		if (ar == NULL)
			return nast_errmsgs[NAST_UNKNOWN_RESPONSE];

		aa = build_result(s, ar->buffer, ar->bufflen);
		if (aa == NULL || aa->nitems == 0)
			return nast_errmsgs[NAST_UNKNOWN_RESPONSE];

		if (ar->errmsg != NULL)
			free(ar->errmsg);

		ar->errmsg = malloc(aa->items[0]->strlen);
		if (ar->errmsg == NULL)
			return nast_errmsgs[NAST_UNKNOWN_RESPONSE];

		memcpy(ar->errmsg, aa->items[0]->strdata, aa->items[0]->strlen);
		nast_free_result(aa);

		return ar->errmsg;
	}

	return nast_errmsgs[ec];
}
