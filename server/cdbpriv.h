#ifndef CDBPRIV_H
#define CDBPRIV_H

#include "conf.h"

#include <sys/types.h>

int cdb_find(char *buff, off_t bufflen, const unsigned char *key,
             unsigned int len, char **ret, uint32_t *retlen);
uint32_t cdb_hash(const unsigned char *buff, unsigned int len);
uint32_t cdb_unpack(unsigned char *buff);

#endif
