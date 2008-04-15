/* $Id: compat.h,v 1.4 2000/03/21 19:06:58 shmit Exp $ */

#ifndef COMPAT_H
#	define COMPAT_H

#include <netdb.h>
#include <syslog.h>
#include <sys/param.h>
#include <sys/types.h>
#include <netinet/in.h>

#if __GNUC__
#define RCSID(x) static const char rcsid[] __attribute__((__unused__)) = x
#else
#define RCSID(x) static const char rcsid[] = x
#endif

#if OS_SOLARIS
#define NEED_SNPRINTF 1
#define NEED_SHADOW 1
#define NEED_CRYPT 1
#define NEED_SETPROCTITLE 1
#define NEED_STRINGS 1

#if SOLARIS_55
#define NEED_CRYPT 1
#include <pthread.h>
#endif
#endif /* OS_SOLARIS */

#if OS_FREEBSD
#include <libutil.h>

#if FREEBSD_2
typedef u_int32_t uint32_t;
#endif /* FREEBSD_2 */
#endif /* OS_FREEBSD */

#if OS_LINUX
#define NEED_SETPROCTITLE 1
#endif /* OS_LINUX */

#ifndef LOG_PERROR
#define LOG_PERROR 0
#endif

#ifndef IPPORT_HIFIRSTAUTO
#define IPPORT_HIFIRSTAUTO 49152
#endif

#ifndef IPPORT_HILASTAUTO
#define IPPORT_HILASTAUTO 65535
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 1024
#endif

#if NEED_SETPROCTITLE
void setproctitle(const char *fmt, ...);
#endif

#if NEED_SNPRINTF
int snprintf(char *str, size_t size, const char *fmt, ...);
#endif

#if NEED_MEMCPY
void *memcpy(void *dst, const void *src, size_t len);
#endif

#if NEED_MEMSET
void *memcpy(void *dst, char c, size_t len);
#endif

#if NEED_SETPROCTITLE
void setproctitle(const char *fmt, ...);
#endif

#endif
