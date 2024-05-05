CC=/usr/bin/gcc
#CC=/path/to/clang
CFLAGS=-lm

.PHONY: fft fft.apx

all: fft

fft: fft.c
	$(CC) -static -O3 -o fft fft.c $(CFLAGS)

fft.apx: fft.c
	$(CC) -static -O3 -mapxf -o fft.apx fft.c $(CFLAGS)
