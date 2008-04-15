#include "conf.h"
#include "log.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

extern char *progname;

#if 0
FILE *logf = NULL;
#endif

static void log_priority(int priority, const char *fmt, va_list args);

/*
 * Log a message with a given priority.
 */
void
log_priority(int priority, const char *fmt, va_list args)
{
	char lbuff[1024];
#if 0
	char tbuff[256];
	struct tm timeptr;
	time_t tloc;
#endif

	(void)snprintf(lbuff, sizeof(lbuff),
		       "[tid: %d] %s", (int)pthread_self(), fmt);
	vsyslog(priority, lbuff, args);

#if 0
	tloc = time(0);
	(void)localtime_r(&tloc, &timeptr);
	(void)strftime(tbuff, sizeof(tbuff), "%Y-%h-%d %H:%M:%S", &timeptr);
	(void)snprintf(lbuff, sizeof(lbuff), "%s %s[%d:%d] %s\n",
		       tbuff, progname, getpid(), (int)pthread_self(), fmt);
	(void)vfprintf(logf, lbuff, args);
#endif
}

/*
 * Log an error message.
 */
void
log_err(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_priority(LOG_ERR, fmt, ap);
	va_end(ap);
}

/*
 * Log a warning message.
 */
void
log_warn(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_priority(LOG_WARNING, fmt, ap);
	va_end(ap);
}

/*
 * Log an informational message.
 */
void
log_info(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_priority(LOG_INFO, fmt, ap);
	va_end(ap);
}

/*
 * Initialize logging facilites.
 */
void
log_open()
{
	openlog(progname, LOG_PID|LOG_PERROR, LOG_DAEMON);

#if 0
	logf = fopen("/home/shmit/var/log/nastd.log", "a+");
#endif
}
