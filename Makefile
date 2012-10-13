# MAC_BUILD - set this to "universal" to build a 2-way fat library 
# CFLAGS - to taste
# CC - C compiler, cc, gcc or clang
#
MAC_BUILD=universal
CFLAGS= #-Wall -pedantic -O2
ifndef CC
CC=cc
endif

# if ARCH set, disable universal build on the mac
ifdef ARCH
MAC_BUILD=
else
ARCH=$(shell uname -m)
endif

UNAME=$(shell uname)

ifeq ($(UNAME), SunOS)
PATH +=:/usr/perl5/5.10.0/bin
CFLAGS +=-fPIC
ifeq ($(ARCH), i86pc)
ARCH=$(shell isainfo -k)
ifeq ($(ARCH), amd64)
ARCH=x86_64
else
ARCH=i386
endif
endif
ifeq ($(ARCH), x86_64)
CFLAGS += -m64
endif
endif

ifeq ($(UNAME), FreeBSD)
CFLAGS += -Wno-error=unknown-pragmas -I/usr/src/sys/cddl/compat/opensolaris -I/usr/src/sys/cddl/contrib/opensolaris/uts/common
ifeq ($(ARCH), i386)
CFLAGS += -m32
endif
endif

ifeq ($(UNAME), Darwin)
ifeq ($(MAC_BUILD), universal)
CFLAGS += -arch i386 -arch x86_64 
else
CFLAGS += -arch $(ARCH)
endif
endif

# main library build
objects = usdt.o usdt_dof_file.o usdt_tracepoints.o usdt_probe.o usdt_dof.o usdt_dof_sections.o
headers = usdt.h usdt_internal.h

.c.o: $(headers)

all: libusdt.a

libusdt.a: $(objects) $(headers)
	rm -f libusdt.a
	ar cru libusdt.a $(objects) 
	ranlib libusdt.a

# Tracepoints build. 
#
# If on Darwin and building a universal library, manually assemble a
# two-way fat object file from both the 32 and 64 bit tracepoint asm
# files.
#
# Otherwise, just choose the appropriate asm file based on the build
# architecture.

ifeq ($(UNAME), Darwin)
ifeq ($(MAC_BUILD), universal)

usdt_tracepoints_i386.o: usdt_tracepoints_i386.s
	as -arch i386 -o usdt_tracepoints_i386.o usdt_tracepoints_i386.s

usdt_tracepoints_x86_64.o: usdt_tracepoints_x86_64.s
	as -arch x86_64 -o usdt_tracepoints_x86_64.o usdt_tracepoints_x86_64.s

usdt_tracepoints.o: usdt_tracepoints_i386.o usdt_tracepoints_x86_64.o
	lipo -create -output usdt_tracepoints.o usdt_tracepoints_i386.o \
	                                        usdt_tracepoints_x86_64.o

else # Darwin, not universal
usdt_tracepoints.o: usdt_tracepoints_$(ARCH).s
	as -arch $(ARCH) -o usdt_tracepoints.o usdt_tracepoints_$(ARCH).s
endif

else # not Darwin; FreeBSD and Illumos

ifeq ($(ARCH), x86_64)
usdt_tracepoints.o: usdt_tracepoints_x86_64.s
	as --64 -o usdt_tracepoints.o usdt_tracepoints_x86_64.s
endif
ifeq ($(ARCH), i386)
usdt_tracepoints.o: usdt_tracepoints_i386.s
	as --32 -o usdt_tracepoints.o usdt_tracepoints_i386.s
endif

endif

clean:
	rm -f *.gch
	rm -f *.o
	rm -f libusdt.a
	rm -f test_usdt
	rm -f test_usdt32
	rm -f test_usdt64

.PHONY: clean test

# testing

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

