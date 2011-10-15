CFLAGS = -g -O0 -Wall -pedantic
CC=cc

objects = usdt.o
headers = usdt.h

all: libusdt.a test_usdt

libusdt.a: $(objects) $(headers)
	ar cru libusdt.a $(objects) 

test_usdt: libusdt.a test_usdt.o
	$(CC) -g -o test_usdt libusdt.a test_usdt.o

clean:
	rm -f *.gch
	rm -f *.o
	rm -f libusdt.a
	rm -f test_usdt

.PHONY: clean
