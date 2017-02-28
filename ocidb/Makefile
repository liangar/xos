## Makefile.hpux

include $(HOME)/nrtrde/Makefile.base

OBJS = column.o parameter.o error.o resultset.o statement.o connection.o

all: oralib

oralib: ${OBJS}
	ar cruv ${LIB_XOS} ${OBJS}

.cpp.o:
	$(CC) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f ${OBJS}
