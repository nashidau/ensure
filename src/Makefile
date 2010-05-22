PKGS=elementary

CFLAGS+=`pkg-config --cflags ${PKGS}` 
LDFLAGS+=`pkg-config --libs ${PKGS}` -rdynamic -ldl

OBJECTS=			\
		ensure.o	\
		enasn.o		\
		enobj.o

ASSURANCES=			\
		assurances/premul.asn	

TARGET=ensure
LIBTARGET=libensure.so.0

all: ${TARGET} ${LIBTARGET} ${ASSURANCES}

${TARGET}: ${OBJECTS}

%.asn: %.c
	${CC} -I.. -Os -fPIC -o $@ -shared -Wl,-soname,$@ $< ${CFLAGS} ${LDFLAGS}

libensure.so.0: libensure.c
	${CC} -fPIC -Os -Wall -shared -Wl,-soname,${LIBTARGET} -o ${LIBTARGET} \
		`pkg-config --cflags --libs ecore evas`	\
		libensure.c

scan: clean
	scan-build make -j 4

clean:
	rm -rf ${TARGET} *.o core

