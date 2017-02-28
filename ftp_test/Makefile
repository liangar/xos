SYSTYPE = HPUX
O = o
CPP = cpp
CC = aCC
LD = aCC
STRIP = strip

FTPDIR = libftp-5.0

CFLAGS = -I. -I${FTPDIR} -D__UNIX__ -D__HPUX__ -D_REENTRANT=1 -D_THREAD_SAFE=1 -DHAS_SYSMACHINE
LDFLAGS = -I${FTPDIR} -L${FTPDIR} -ldld -lpthread -lftp
LDFLAGS_2 = -ldld -lpthread

OUTDIR = bin

SVR = ftp_test

SVRSRCS = l_str.cpp iniFile.cpp xsys_ftp.${CPP} xini.${CPP} lfile.${CPP} ${SVR}.${CPP}
SVROBJS = ${SVRSRCS:.${CPP}=.${O}}

.cpp.o:
	${CC} ${CFLAGS} -c $<

all: ${OUTDIR} ${SVR}

${OUTDIR}:
	@mkdir ${OUTDIR}

${SVR}: ${SVROBJS}
	${LD} -o ${SVR} ${SVROBJS} ${LDFLAGS}
	${STRIP} ${SVR}

clean:
	rm -f *.${O} ${SVR}
	rm -rf ${OUTDIR}
