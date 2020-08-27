.PHONY: all clean

all: ask2-signals

CC = gcc
CFLAGS = -g -Wall -O2
DEPS=proc-common.h
SHELL= /bin/bash

ask2-signals: ask2-signals.o tree.o proc-common.o proc-common.h tree.h
	$(CC) $(CFLAGS) $^ -o $@

%.s: %.c
	$(CC) $(CFLAGS) -S -fverbose-asm $<

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

%.i: %.c
	gcc -Wall -E $< | indent -kr > $@

clean:
	rm -f *.o tree-example fork-example pstree-this ask2-{fork,tree,signals,pipes}
