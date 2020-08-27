all: zing zing2

zing:main.o zing.o
	gcc -c main.c -o main.o
	gcc -o zing main.o zing.o

zing2.o:zing2.c zing2.h
	gcc -c zing2.c

zing2: main.o zing2.o
	gcc -c main2.c -o main.o
	gcc -o zing2 main.o zing2.o
