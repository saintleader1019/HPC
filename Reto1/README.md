CC=gcc
CFLAGS=-O3 -march=native -std=c11
MATH=-lm

all: dart_v1_serial buffon_v1_serial

quiet: CFLAGS+=-DQUIET
quiet: all

dart_v1_serial: dart_v1_serial.c common.h
	$(CC) $(CFLAGS) dart_v1_serial.c -o $@

buffon_v1_serial: buffon_v1_serial.c common.h
	$(CC) $(CFLAGS) buffon_v1_serial.c $(MATH) -o $@

clean:
	rm -f dart_v1_serial buffon_v1_serial
