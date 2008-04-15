# $Id: Makefile,v 1.15 2000/12/27 06:51:51 shmit Exp $

LIBASS=	../client/libnast.a

tclients: getnast statnast stressnast randnast mtstressnast updnast	\
	  killnast

GETOBJS= getnast.o
getnast: ${GETOBJS}
	${PURIFY} ${CC} -o $@ ${GETOBJS} ${LIBASS} ${LIBS}

STATOBJS= statnast.o
statnast: ${STATOBJS}
	${PURIFY} ${CC} -o $@ ${STATOBJS} ${LIBASS} ${LIBS}

STRESSOBJS= stressnast.o
stressnast: ${STRESSOBJS}
	${PURIFY} ${CC} -o $@ ${STRESSOBJS} ${LIBASS} ${LIBS}

RANDOBJS= randnast.o
randnast: ${RANDOBJS}
	${PURIFY} ${CC} -o $@ ${RANDOBJS} ${LIBASS} ${LIBS}

MTSTRESSOBJS= mtstressnast.o
mtstressnast: ${MTSTRESSOBJS}
	${PURIFY} ${CC} -o $@ ${MTSTRESSOBJS} ${LIBASS} ${LIBS}

UPDOBJS= updnast.o
updnast: ${UPDOBJS}
	${PURIFY} ${CC} -o $@ ${UPDOBJS} ${LIBASS} ${LIBS}

KILLOBJS= killnast.o
killnast: ${KILLOBJS}
	${PURIFY} ${CC} -o $@ ${KILLOBJS} ${LIBASS} ${LIBS}

#
# Dependencies
#
getnast.o:	../include/nastd.h
statnast.o:	../include/nastd.h
stressnast.o:	../include/nastd.h
randnast.o:	../include/nastd.h
mtstressnast.o:	../include/nastd.h

MKDIR=	../Makefiles
include ${MKDIR}/build
