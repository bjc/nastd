LIBOBJS= compat.o
libcompat.a: ${LIBOBJS}
	ar r $@ ${LIBOBJS}
	ranlib $@

MKDIR=	../Makefiles
include ${MKDIR}/build
