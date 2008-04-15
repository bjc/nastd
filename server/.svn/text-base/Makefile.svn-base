# $Id: Makefile,v 1.17 2001/10/04 23:57:09 shmit Exp $

OBJS=	array.o nastd.o nastdio.o cdb.o cdb_find.o cdb_hash.o		\
	cdb_unpack.o fqm.o log.o md5.o memdb.o mysqldb.o periodic.o	\
	thread.o ../libconfig/libconfig.a

nastd:	${OBJS}
	${PURIFY} ${CC} -o $@ ${OBJS} ${LIBS} ${MYSQL_LIBS}

#
# Dependencies
#
array.o:	array.h log.h
nastd.o:	../include/nastipc.h ../include/config.h  nastdio.h	\
		cdb.h log.h memdb.h mysqldb.h periodic.h
nastdio.o:	../include/nastd.h ../include/nastipc.h array.h		\
		nastdio.h cdb.h fqm.h log.h memdb.h mysqldb.h thread.h
cdb.o:		../include/config.h array.h nastdio.h cdbpriv.h log.h	\
		memdb.h thread.h
cdb_find.o:	cdbpriv.h
cdb_hash.o:	cdbpriv.h
cdb_unpack.o:	cdbpriv.h
fqm.o:		fqm.h log.h thread.h
log.o:		log.h
md5.o:		log.h md5.h
memdb.o:	../include/config.h array.h log.h md5.h memdb.h thread.h
mysqldb.o:	array.h nastdio.h log.h mysqldb.h
periodic.o:	cdb.h log.h thread.h periodic.h
thread.o:	thread.h

MKDIR=	../Makefiles
include ${MKDIR}/build
