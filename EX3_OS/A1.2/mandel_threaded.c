/*
 * mandel-thr.c
 *
 * A program to draw the Mandelbrot Set on a 256-color xterm.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <signal.h>

#include "mandel-lib.h"

#define MANDEL_MAX_ITERATION 100000

#define perror_pthread(ret, msg) \
        do { errno = ret; perror(msg); } while (0)

/***************************
 * Compile-time parameters *
 ***************************/


/*
 * Output at the terminal is is x_chars wide by y_chars long
 */
int y_chars = 50;
int x_chars = 90;

/*
 * The part of the complex plane to be drawn:
 * upper left corner is (xmin, ymax), lower right corner is (xmax, ymin)
 */
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;

/*
 * Every character in the final output is
 * xstep x ystep units wide on the complex plane.
 */
double xstep;
double ystep;

struct thread_info_struct {
	pthread_t tid; /* POSIX thread id, as returned by the library */
	sem_t *sem;

	int thrid; /* Application-defined thread id */
	int thrcnt;
};


/*
 * This function computes a line of output
 * as an array of x_char color values.
 */
void compute_mandel_line(int line, int color_val[])
{
	/*
	 * x and y traverse the complex plane.
	 */
	double x, y;

	int n;
	int val;

	/* Find out the y value corresponding to this line */
	y = ymax - ystep * line;

	/* and iterate for all points on this line */
	for (x = xmin, n = 0; n < x_chars; x+= xstep, n++) {

		/* Compute the point's color value */
		val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
		if (val > 255)
			val = 255;

		/* And store it in the color_val[] array */
		val = xterm_color(val);
		color_val[n] = val;
	}
}

/*
 * This function outputs an array of x_char color values
 * to a 256-color xterm.
 */
void output_mandel_line(int fd, int color_val[])
{
	int i;

	char point ='@';
	char newline='\n';

	for (i = 0; i < x_chars; i++) {
		/* Set the current color, then output the point */
		set_xterm_color(fd, color_val[i]);
		if (write(fd, &point, 1) != 1) {
			perror("compute_and_output_mandel_line: write point");
			exit(1);
		}
	}

	/* Now that the line is done, output a newline character */
	if (write(fd, &newline, 1) != 1) {
		perror("compute_and_output_mandel_line: write newline");
		exit(1);
	}
}

void compute_and_output_mandel_line(int fd, int line)
{
	/*
	 * A temporary array, used to hold color values for the line being drawn
	 */
	int color_val[x_chars];

	compute_mandel_line(line, color_val);
	output_mandel_line(fd, color_val);
}

void *safe_malloc(size_t size)
{
	void *p;

	if ((p = malloc(size)) == NULL) {
		fprintf(stderr, "Out of memory, failed to allocate %zd bytes\n",
				size);
		exit(1);
	}

	return p;
}

int safe_atoi(char *s, int *val)
{
	long l;
	char *endp;

	l = strtol(s, &endp, 10);
	if (s != endp && *endp == '\0') {
		*val = l;
		return 0;
	} else
		return -1;
}

/* Start function for each thread */
void *thread_function (void *arg) {
	int i, current, next, color_val[x_chars];

	struct thread_info_struct *thr = arg;
	current = thr->thrid;
	next = (current + 1) % thr->thrcnt;

	/* Call sem_wait on current thread's semaphore and sem_post on next thread's semaphore */
	for (i = thr->thrid; i < y_chars; i += thr->thrcnt) {
		compute_mandel_line(i, color_val);

		sem_wait(&thr->sem[current]);

		output_mandel_line(1, color_val);

		//compute_and_output_mandel_line(1, i);

		if (next < thr->thrcnt) {
			sem_post(&thr->sem[next]);
		}
	}

	return NULL;
}

void sigint_handler() {
	reset_xterm_color(1);
	exit(1);
}

int main(int argc, char* argv[])
{
	int thrcnt, i, ret, temp;
	struct thread_info_struct *thr;
	sem_t *sem;

	struct sigaction sa;

	sa.sa_handler = sigint_handler;
	sigaction(SIGINT, &sa, NULL);

	if(argc != 2) {
		fprintf(stderr, "Usage: %s <number_of_threads>.\n", argv[0]);
		exit(1);
	}

	if (safe_atoi(argv[1], &thrcnt) < 0 || thrcnt <= 0) {
		fprintf(stderr, "`%s' is not valid for `thread_count'\n", argv[1]);
		exit(1);
	}

	thr = safe_malloc(thrcnt * sizeof(*thr));
	sem = safe_malloc(thrcnt * sizeof(*sem));

	xstep = (xmax - xmin) / x_chars;
	ystep = (ymax - ymin) / y_chars;

	/* Initialize semaphores */
	for (i = 0; i < thrcnt; i++) {
		if (i == 0) {
			temp = 1;
		}
		else {
			temp = 0;
		}

		if (sem_init(&sem[i], 0, temp) == -1) {
			fprintf(stderr, "Failed to create semaphore.\n");
		}
	}

	for (i = 0; i < thrcnt; i++) {
		thr[i].thrid = i;
		thr[i].thrcnt = thrcnt;
		thr[i].sem = sem;

		/* Spawn new thread */
		ret = pthread_create(&thr[i].tid, NULL, thread_function, &thr[i]);
		if (ret) {
			perror_pthread(ret, "pthread_create");
			exit(1);
		}

	}

	/* Wait for all threads to terminate */
	for (i = 0; i < thrcnt; i++) {
		ret = pthread_join(thr[i].tid, NULL);
		if (ret) {
			perror_pthread(ret, "pthread_join");
			exit(1);
		}
	}

	reset_xterm_color(1);
	return 0;
}
