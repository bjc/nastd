/* $Id: memdb.h,v 1.8 2000/05/17 19:32:58 shmit Exp $ */

#ifndef MEMDB_H
#define MEMDB_H

int memdb_new();
void memdb_delete();

int memdb_add(const char *key, int keylen, array_t *vals);
int memdb_del(const char *key, int keylen);
int memdb_get(const char *key, int keylen, array_t *vals);
int memdb_upd(const char *key, int keylen, array_t *vals);

int memdb_stats(array_t *statarr);
void memdb_collate();

#endif
