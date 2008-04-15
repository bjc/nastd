#include "cdbpriv.h"

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

RCSID("$Id: cdb_find.c,v 1.2 2000/02/29 19:31:33 shmit Exp $");

static int
cdb_bread(char **ptr, char *endptr, char *buf, unsigned int len)
{
	if ((*ptr)+len > endptr) {
		errno = EIO;
		return -1;
	}

	memcpy(buf, *ptr, len);
	*ptr += len;
	return 0;
}

static int
match(char **ptr, char *endptr, const char *key, unsigned int len)
{
	char buf[32];
	int n;
	int i;

	n = sizeof(buf);
	if (n > len)
		n = len;

	while (len > 0) {
		if (cdb_bread(ptr, endptr, buf, n) == -1)
			return -1;

		for (i = 0; i < n; ++i)
			if (buf[i] != key[i])
				return 0;
		key += n;
		len -= n;
	}

	return 1;
}

int
cdb_find(char *buff, off_t bufflen, const char *key, int len,
	 char **ret, uint32_t *retlen)
{
	char *cur, *end;
	char packbuf[8];
	uint32_t pos;
	uint32_t h;
	uint32_t lenhash;
	uint32_t h2;
	uint32_t loop;
	uint32_t poskd;

	cur = buff;
	end = buff + bufflen;

	h = cdb_hash(key, len);

	pos = 8 * (h & 255);
	cur += pos;
	if (cur > end) {
		errno = EIO;
		return -1;
	}

	if (cdb_bread(&cur, end, packbuf, 8) == -1)
		return -1;

	pos = cdb_unpack(packbuf);
	lenhash = cdb_unpack(packbuf + 4);

	if (!lenhash) return 0;
	h2 = (h >> 8) % lenhash;

	for (loop = 0; loop < lenhash; ++loop) {
		cur = buff + (pos + 8 * h2);
		if (cur > end) {
			errno = EIO;
			return -1;
		}
		if (cdb_bread(&cur, end, packbuf, 8) == -1)
			return -1;
		poskd = cdb_unpack(packbuf + 4);
		if (!poskd)
			return 0;

		if (cdb_unpack(packbuf) == h) {
			cur = buff + poskd;
			if (cur > end) {
				errno = EIO;
				return -1;
			}
			if (cdb_bread(&cur, end, packbuf, 8) == -1)
				return -1;
			if (cdb_unpack(packbuf) == len) {
				switch(match(&cur, end, key, len)) {
				case -1:
					return -1;
				case 1:
					*retlen = cdb_unpack(&packbuf[4]);
					*ret = cur;
					return 1;
				}
			}
		}
		if (++h2 == lenhash)
			h2 = 0;
	}

	return 0;
}
