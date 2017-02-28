## Makefile.hpux

include $(HOME)/nrtrde/Makefile.base

OBJS = SysDep.o Errors.o 

all: libxmailsys

libxmailsys: ${OBJS}
	ar cruv ${LIB_XOS} ${OBJS}

.cpp.o:
	${CC} ${CFLAGS} -c $<
.c.o:
	${CC} ${CFLAGS} -c $<

clean:
	rm -f ${OBJS}
