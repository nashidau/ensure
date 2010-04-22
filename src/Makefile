PKGS=elementary

CFLAGS+=`pkg-config --cflags ${PKGS}`
LDFLAGS+=`pkg-config --libs ${PKGS}`

OBJECTS=			\
		ensure.o

TARGET:ensure

${TARGET}: ${OBJECTS}

ensure.o: 

scan: clean
	scan-build make -j 4

clean:
	rm -rf ${TARGET} *.o core

