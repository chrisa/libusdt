UNAME=$(shell uname)
ARCH=$(shell uname -m)

MAC_BUILD=universal
CFLAGS= -g -O0 -Wall -Werror
CC=cc

ifeq ($(UNAME), SunOS)
PATH +=:/usr/perl5/5.10.0/bin
CFLAGS +=-fPIC
ifeq ($(ARCH), x86_64)
CFLAGS += -m64
endif
endif

ifeq ($(UNAME), FreeBSD)
CFLAGS += -Wno-error=unknown-pragmas  -I/usr/src/sys/cddl/compat/opensolaris -I/usr/src/sys/cddl/contrib/opensolaris/uts/common
ifeq ($(ARCH), i386)
CFLAGS += -m32
endif
endif

objects = usdt.o usdt_dof_file.o usdt_tracepoints.o usdt_probe.o usdt_dof.o usdt_dof_sections.o
headers = usdt.h usdt_internal.h

.c.o: $(headers)

all: libusdt.a

ifeq ($(UNAME), Darwin)
ifeq ($(MAC_BUILD), universal)
CFLAGS+= -arch i386 -arch x86_64

usdt_tracepoints_i386.o: usdt_tracepoints_i386.s
	as -arch i386 -o usdt_tracepoints_i386.o usdt_tracepoints_i386.s

usdt_tracepoints_x86_64.o: usdt_tracepoints_x86_64.s
	as -arch x86_64 -o usdt_tracepoints_x86_64.o usdt_tracepoints_x86_64.s

usdt_tracepoints.o: usdt_tracepoints_i386.o usdt_tracepoints_x86_64.o
	lipo -create -output usdt_tracepoints.o usdt_tracepoints_i386.o \
	                                        usdt_tracepoints_x86_64.o
else

ifeq ($(ARCH), x86_64)
usdt_tracepoints.o: usdt_tracepoints_x86_64.s
	as -o usdt_tracepoints.o usdt_tracepoints_x86_64.s
else
usdt_tracepoints.o: usdt_tracepoints_i386.s
	as -o usdt_tracepoints.o usdt_tracepoints_i386.s
endif

endif
endif

ifeq ($(UNAME), FreeBSD)
ifeq ($(ARCH), amd64)
usdt_tracepoints.o: usdt_tracepoints_x86_64.s
	as --64 -o usdt_tracepoints.o usdt_tracepoints_x86_64.s
else
usdt_tracepoints.o: usdt_tracepoints_i386.s
	as --32 -o usdt_tracepoints.o usdt_tracepoints_i386.s
endif
endif

ifeq ($(UNAME), SunOS)
ifeq ($(ARCH), x86_64)
usdt_tracepoints.o: usdt_tracepoints_x86_64.s
	as --64 -o usdt_tracepoints.o usdt_tracepoints_x86_64.s
else
usdt_tracepoints.o: usdt_tracepoints_i386.s
	as --32 -o usdt_tracepoints.o usdt_tracepoints_i386.s
endif
endif

libusdt.a: $(objects) $(headers)
	ar cru libusdt.a $(objects) 
	ranlib libusdt.a

ifeq ($(UNAME), Darwin)
ifeq ($(MAC_BUILD), universal)
test_usdt64: libusdt.a test_usdt.o
	$(CC) -arch x86_64 -o test_usdt64 test_usdt.o libusdt.a
test_usdt32: libusdt.a test_usdt.o
	$(CC) -arch i386 -o test_usdt32 test_usdt.o libusdt.a
else
test_usdt: libusdt.a test_usdt.o
	$(CC) $(CFLAGS) -o test_usdt test_usdt.o libusdt.a 
endif
else
test_usdt: libusdt.a test_usdt.o
	$(CC) $(CFLAGS) -o test_usdt test_usdt.o libusdt.a 
endif

clean:
	rm -f *.gch
	rm -f *.o
	rm -f libusdt.a
	rm -f fat_libusdt.a
	rm -f test_usdt
	rm -f test_usdt32
	rm -f test_usdt64

ifeq ($(UNAME), Darwin)
ifeq ($(MAC_BUILD), universal)
test: test_usdt32 test_usdt64
	sudo prove test.pl :: 64
	sudo prove test.pl :: 32
else
test: test_usdt
	sudo prove test.pl
endif
else
test: test_usdt
	sudo prove test.pl
endif

.PHONY: clean test
