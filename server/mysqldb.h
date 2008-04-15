#ifndef MYSQLDB_H
#define MYSQLDB_H

#include "array.h"
#include "fqm.h"

int mysqldb_new();
void *mysqldb_connect_new();
void mysqldb_connect_close(void *dbh);
int mysqldb_get(reqthread_t *self, const char *key, int keylen, array_t *aa);

int mysqldb_stats(array_t *statarr);
void mysqldb_collate();

#endif
