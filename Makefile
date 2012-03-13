MAC_BUILD=universal
CFLAGS= -g -O0 -Wall -Werror -arch i386 -arch x86_64
CC=cc
ARCH=$(shell uname -m)

objects = usdt.o usdt_dof_file.o usdt_probe.o usdt_tracepoints.o
headers = usdt.h

.c.o: $(headers)

all: libusdt.a test_usdt

ifeq ($(MAC_BUILD), universal)
usdt_tracepoints_i386.o:
	as -arch i386 -o usdt_tracepoints_i386.o usdt_tracepoints_i386.s

usdt_tracepoints_x86_64.o:
	as -arch x86_64 -o usdt_tracepoints_x86_64.o usdt_tracepoints_x86_64.s

usdt_tracepoints.o: usdt_tracepoints_i386.o usdt_tracepoints_x86_64.o
	lipo -create -output usdt_tracepoints.o usdt_tracepoints_i386.o \
	                                        usdt_tracepoints_x86_64.o
else

ifeq ($(ARCH), x86_64)
usdt_tracepoints.o:
	as -o usdt_tracepoints.o usdt_tracepoints_x86_64.s
else
usdt_tracepoints.o:
	as -o usdt_tracepoints.o usdt_tracepoints_i386.s
endif

endif

fat_libusdt.a: $(objects) $(headers)
	ar cru fat_libusdt.a $(objects) 

libusdt.a: fat_libusdt.a
	cp fat_libusdt.a libusdt.a
	ranlib -s libusdt.a

ifeq ($(MAC_BUILD), universal)
test_usdt: libusdt.a test_usdt.o
	$(CC) -arch i386 -arch x86_64 -o test_usdt libusdt.a test_usdt.o
else
test_usdt: libusdt.a test_usdt.o
	$(CC) -o test_usdt libusdt.a test_usdt.o
endif

clean:
	rm -f *.gch
	rm -f *.o
	rm -f libusdt.a
	rm -f fat_libusdt.a
	rm -f test_usdt

test: test_usdt
	sudo prove test.pl

.PHONY: clean test
