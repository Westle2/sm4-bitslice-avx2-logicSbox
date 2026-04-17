CC=gcc
CFLAGS=-O3 -mavx2 -Iinclude

all:
	$(CC) $(CFLAGS) src/*.c -o sm4_bench

clean:
	rm -f sm4_bench
