
include ../../include.mk
include ../../version.mk

RELPATH = ../../

INCLUDE_DIR = ${RELPATH}include
CFLAGS   := ${CFLAGS} -I${INCLUDE_DIR}
CPPFLAGS := ${CPPFLAGS} -I${INCLUDE_DIR}
LFLAGS   := ${RELPATH}lib/libmcbsp${VERSION}.a -lm ${LFLAGS}

all: ${RELPATH}lib/libmcbsp${VERSION}.a file_io_cpr

${RELPATH}lib/libmcbsp${VERSION}.a:
	(cd ${RELPATH};make)

file_io_cpr: file_io_cpr.o
	${CC} ${CFLAGS} -o $@ $^ ${LFLAGS}

clean:
	rm -f file_io_cpr

