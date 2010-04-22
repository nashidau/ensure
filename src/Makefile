PKGS=elementary

CFLAGS+=`pkg-config --cflags ${PKGS}`
LDFLAGS+=`pkg-config --libs ${PKGS}`

OBJECTS=			\
		ensure.o

TARGET=ensure
LIBTARGET=libensure.so.0

all: ${TARGET} ${LIBTARGET}

${TARGET}: ${OBJECTS}

ensure.o: 

libensure.so.0: libensure.c
	${CC} -fPIC -Os -Wall -shared -Wl,-soname,${LIBTARGET} -o ${LIBTARGET} \
		`pkg-config --cflags --libs ecore evas`	\
		libensure.c

scan: clean
	scan-build make -j 4

clean:
	rm -rf ${TARGET} *.o core

