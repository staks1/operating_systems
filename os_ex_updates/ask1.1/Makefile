CC=gcc
CFLAGS=-I.

main.o:	main.c
	$(CC) -c main.c

zing2.o:zing.c
	$(CC) -c zing.c -o zing2.o

zing:	main.o zing.o zing.h
	$(CC) -o zing main.o zing.o

zing2:	main.o zing2.o
	$(CC) -o zing2 main.o zing2.o
