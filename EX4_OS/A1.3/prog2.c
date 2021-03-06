#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "proc-common.h"

#define NMSG 200
#define DELAY 130




int main(int argc, char *argv[])
{
     //set up the alarm that the handler must handle (default action is termination !)
		 printf("Hi !!I am the given executable \n");
			printf("I child[%ld] : set up the alarm and i am starting \n !",getpid());
			int i, delay, pid;



					/*
					 * Print a number of messages,
					 * use a random delay, so that processes terminate
					 * in random order.
					 */

					pid = getpid();
					srand(pid);
					delay = 30 + ((double)rand() / RAND_MAX) * DELAY;
					printf("%s: Starting, NMSG = %d, delay = %d\n",
						argv[0], NMSG, delay);

					for (i = 0; i < NMSG; i++) {
						printf("%s[%d]: This is message %d\n", argv[0], pid, i);
						compute(delay);
					}


	return 0;
}
