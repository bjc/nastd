#ifndef MEMDB_H
#define MEMDB_H

int memdb_new();
void memdb_delete();

int memdb_add(const unsigned char *key, int keylen, array_t *vals);
int memdb_del(const unsigned char *key, int keylen);
int memdb_get(const unsigned char *key, int keylen, array_t *vals);
int memdb_upd(const unsigned char *key, int keylen, array_t *vals);

int memdb_stats(array_t *statarr);
void memdb_collate();

#endif
