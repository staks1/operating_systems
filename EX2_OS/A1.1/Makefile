.PHONY: all clean

all: ask2.1

CC = gcc
CFLAGS = -g -Wall -O2
SHELL= /bin/bash

ask2.1: ask2.1.o proc-common.o
	$(CC) $(CFLAGS) $^ -o $@

%.s: %.c
	$(CC) $(CFLAGS) -S -fverbose-asm $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.i: %.c
	gcc -Wall -E $< | indent -kr > $@

clean:
	rm -f *.o  ask2.1
