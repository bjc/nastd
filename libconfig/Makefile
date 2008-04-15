CONFIGOBJS= ArrayData.o BuffIO.o Data.o DictData.o NumData.o Parser.o	\
	    String.o StringData.o config.o

libconfig.a: ${CONFIGOBJS}
	ar r $@ ${CONFIGOBJS}
	ranlib $@

MKDIR=	../Makefiles
include ${MKDIR}/build
