.PHONY: all clean

all:ask1.2

CC = gcc
CFLAGS = -g -Wall -O2
SHELL= /bin/bash


ask1.2: ask1.2.o tree.o proc-common.o proc-common.h
	$(CC) $(CFLAGS) $^ -o $@

%.s: %.c
	$(CC) $(CFLAGS) -S -fverbose-asm $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.i: %.c
	gcc -Wall -E $< | indent -kr > $@

clean:
	rm -f *.o tree-example fork-example pstree-this ask2-{fork,tree,signals,pipes}
