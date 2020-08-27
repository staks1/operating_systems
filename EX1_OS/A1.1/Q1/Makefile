CC=gcc
CFLAGS=-I.
DEPS=	zing.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main.o:	main.c
	$(CC) -c main.c
zing:	main.o zing.o
	$(CC) -o zing main.o zing.o
