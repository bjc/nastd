/* $Id: nastd.h,v 1.7 2001/10/29 11:17:12 shmit Exp $ */

#ifndef NASTD_H
#define NASTD_H

enum _nast_bool { NASTFALSE = 0, NASTTRUE = 1 };
typedef enum _nast_bool nast_bool;

enum _errcodes {
	NAST_OK = 0,
	NAST_SERVER_GONE = 1,
	NAST_NOMEM = 2,
	NAST_UNKNOWN_RESPONSE = 3,
	NAST_TIMEDOUT = 4,
	NAST_UNKNOWN_OPT = 5,
	NAST_SERVER_ERR
};
typedef enum _errcodes errcodes;

struct _nast_options {
	nast_bool use_qcache;
	nast_bool use_localdb;
	nast_bool fallthrough_async;
	nast_bool always_fallthrough;
	nast_bool fail_once;
	nast_bool no_fallthrough;
};
typedef struct _nast_options nast_options;

struct _nast_string_t {
	char *strdata;
	int strlen;
};
typedef struct _nast_string_t nast_string_t;

struct _nast_array {
	int nitems;
	nast_string_t **items;
};
typedef struct _nast_array nast_array;

struct _nast_response {
	char *buffer;
	char *errmsg;
	errcodes errcode;
	short bufflen;
	unsigned short reqid;
};
typedef struct _nast_response nast_response;

struct _nast_sphincter {
	nast_response **responses;
	void *lock;
	int socket;
	int nthreads;
};
typedef struct _nast_sphincter nasth;

nasth *nast_sphincter_new(const char *sock_path);
int nast_sphincter_connect(nasth *sphincter, const char *hostname,
			   unsigned short port, const char *username,
			   const char *password);
void nast_sphincter_close(nasth *sphincter);

nast_array *nast_get_result(nasth *sphincter);
void nast_free_result(nast_array *result);

nast_array *nast_array_new();
int nast_array_add(nast_array *array, short len, const char *data);
void nast_array_delete(nast_array *array);

int nast_options_set(nasth *sphincter, nast_options *opts);
int nast_options_get(nasth *sphincter, nast_options *opts);

int nast_add(nasth *sphincter, const char *query);
int nast_del(nasth *sphincter, const char *query);
int nast_get(nasth *sphincter, const char *query);
int nast_upd(nasth *sphincter, const char *key, nast_array *valarray);

int nast_stats(nasth *sphincter);

errcodes nast_geterr(nasth *sphincter);
char *nast_errmsg(nasth *sphincter);

#endif
