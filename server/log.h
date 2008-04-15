/* $Id: log.h,v 1.1 2000/02/29 00:32:12 shmit Exp $ */

#ifndef LOG_H
#	define LOG_H

void log_open();
void log_err(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_info(const char *fmt, ...);

#endif /* LOG_H */
