# $Id: Makefile,v 1.2 2000/03/07 22:55:43 shmit Exp $

LIBOBJS= compat.o
libcompat.a: ${LIBOBJS}
	ar r $@ ${LIBOBJS}
	ranlib $@

MKDIR=	../Makefiles
include ${MKDIR}/build
