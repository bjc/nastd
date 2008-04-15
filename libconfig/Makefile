# $Id: Makefile,v 1.1 2001/10/04 23:57:55 shmit Exp $

CONFIGOBJS= ArrayData.o BuffIO.o Data.o DictData.o NumData.o Parser.o	\
	    String.o StringData.o config.o

libconfig.a: ${CONFIGOBJS}
	ar r $@ ${CONFIGOBJS}
	ranlib $@

MKDIR=	../Makefiles
include ${MKDIR}/build
