#ifndef CDB_H
#define CDB_H

#include "array.h"

int cdb_new();
int cdb_get(const char *key, int keylen, array_t *dstarr);
void cdb_periodic();

int cdb_stats(array_t *statarr);
void cdb_collate();

#endif
