#include "compat.h"

#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>

RCSID("$Id: compat.c,v 1.1 2000/03/07 22:39:27 shmit Exp $");

#if NEED_SETPROCTITLE
void
setproctitle(const char *fmt, ...)
{
}
#endif

#if NEED_SNPRINTF
int
snprintf(char *str, size_t size, const char *fmt, ...)
{
	va_list ap;
	int rc;

	va_start(ap, fmt);
	rc = vsprintf(str, fmt, ap);
	va_end(ap);

	return rc;
}
#endif

#if NEED_MEMCPY
void *
memcpy(void *dst, const void *src, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		(char *)dst+i = (char *)src+i;

	return dst;
}
#endif

#if NEED_MEMSET
void *
memset(void *dst, char c, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		(char *)dst+i = (char *)src+i;

	return dst;
}
#endif
