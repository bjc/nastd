# $Id: Makefile,v 1.10 2001/01/19 02:54:37 shmit Exp $

LIBOBJS= nastapi.o thread.o

all-lib:
	@rm -f *.o
	@make "THREADFLAGS=-UTHREADSAFECLIENT" libnast.a
	@rm -f *.o
	@make "THREADFLAGS=-DTHREADSAFECLIENT" libnast_r.a

libnast.a: ${LIBOBJS}
	ar r $@ ${LIBOBJS}
	ranlib $@

libnast_r.a: ${LIBOBJS}
	ar r $@ ${LIBOBJS}
	ranlib $@

#
# Dependencies
#
nastapi.o:	../include/nastd.h ../include/nastipc.h thread.h
thread.o:	thread.h

MKDIR=	../Makefiles
include ${MKDIR}/build
