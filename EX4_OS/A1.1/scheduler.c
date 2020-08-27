#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"
#include "request.h"

/* Compile-time parameters. */
#define SCHED_TQ_SEC 2  /* time quantum */
#define TASK_NAME_SZ 10 /* maximum size for a task's name */
#define SLEEP_SEC 1
#define ALARM_SEC 2

typedef struct ProcessList {
  pid_t procid;
  struct ProcessList *next;
  char procname[10];
  int TaskId;
} ProcessList;

struct queue {
  ProcessList *items; // array to store queue elements
  int maxsize;        // maximum capacity of the queue
  int front;          // front points to front element in the queue (if any)
  int rear;           // rear points to last element in the queue
  int size;           // current capacity of the queue
};

// Utility function to initialize queue
struct queue *newProcessQueue(int size) {
  struct queue *pt = NULL;
  pt = (struct queue *)malloc(sizeof(struct queue));

  pt->items = (ProcessList *)malloc(size * sizeof(ProcessList));
  pt->maxsize = size;
  pt->front = 0;
  pt->rear = -1;
  pt->size = 0;

  return pt;
}

// Utility function to return the size of the queue
int size(struct queue *pt) { return pt->size; }

// Utility function to check if the queue is empty or not
int isEmpty(struct queue *pt) { return !size(pt); }

// Utility function to return front element in queue
ProcessList front(struct queue *pt) {
  if (isEmpty(pt)) {
    printf("UnderFlow\nProgram Terminated\n");
    exit(EXIT_FAILURE);
  }

  return pt->items[pt->front];
}

/* function to enqueue  process */
void enqueue(struct queue *pt, ProcessList x) {
  if (size(pt) == pt->maxsize) {
    printf("OverFlow\nProgram Terminated\n");
    exit(EXIT_FAILURE);
  }

  // printf("Inserting process %s  with pid %ld and Id %d at the end of the
  // queue \n", x.procname,x.procid,x.TaskId);

  pt->rear = (pt->rear + 1) % pt->maxsize; // circular queue
  pt->items[pt->rear] = x;
  pt->size++;

  // printf("front = %d, rear = %d\n", pt->front, pt->rear);
}

/*function to dequeue process */
void dequeue(struct queue *pt) {
  if (isEmpty(pt)) // front == rear
  {
    printf("UnderFlow\nProgram Terminated\n");
    exit(EXIT_FAILURE);
  }

  printf("Removing  process %s  with pid: %ld \n", (front(pt)).procname,
         (front(pt)).procid);

  pt->front = (pt->front + 1) % pt->maxsize; // circular queue
  pt->size--;

  // printf("front = %d, rear = %d\n", pt->front, pt->rear);
}

/* finished Queue implementation  */

/*handlers and main() */

/*initializing global pointers */
pid_t cur_pid = 0;
ProcessList *current;
struct queue *pt = NULL;
ProcessList *process = NULL;
pid_t *pid = NULL;

/*
 * SIGALRM handler
 */
static void sigalrm_handler(int signum) {
  if (signum != SIGALRM) {
    fprintf(stderr, "Internal error: Called for signum %d, not SIGALRM\n",
            signum);
    exit(1);
  }

  printf("ALARM! %d seconds have passed.\n", ALARM_SEC);

  // sigstop the process and put at the end of the queue
  printf("This child [ %ld ]  must now be put on hold (back of the queue).\n",
         cur_pid);
  kill(cur_pid, SIGSTOP);
}

/*
 * SIGCHLD handler
 */
static void sigchld_handler(int signum) {

  pid_t p;
  int status;

  if (signum != SIGCHLD) {
    fprintf(stderr, "Internal error: Called for signum %d, not SIGCHLD\n",
            signum);
    exit(1);
  }

  /*
   * Something has happened to one of the children.
   * We use waitpid() with the WUNTRACED flag, instead of wait(), because
   * SIGCHLD may have been received for a stopped, not dead child.
   *
   * A single SIGCHLD may be received if many processes die at the same time.
   * We use waitpid() with the WNOHANG flag in a loop, to make sure all
   * children are taken care of before leaving the handler.
   */

  for (;;) {
    p = waitpid(-1, &status, WUNTRACED | WNOHANG);
    if (p < 0) {
      perror("waitpid");
      exit(1);
    }
    if (p == 0)
      break;

    explain_wait_status(p, status);

    if (WIFEXITED(status) || WIFSIGNALED(status)) {
      /* A child has died */
      printf("Parent: Received SIGCHLD, child is dead.\n");

      // remove from queue the child that finished
      // if it was the last one --> all processes finished
      if (!isEmpty(pt)) {
        dequeue(pt);
        // printf(" queue size %d\n: ",size(pt));
      }

      // this is just a guarding else -->should not go here
      else {
        printf("All processes have ended !Exiting scheduler...\n");
        exit(1);
      }

      // if after removing the last process the queue is empty then we are done
      // !
      if (isEmpty(pt)) {
        printf("All processes have ended !Exiting scheduler...\n");
        exit(1);
      }

      printf(" New child [ %ld ] is going to run now !\n", front(pt).procid);

      // wake up the next process
      cur_pid = front(pt).procid;
      kill(cur_pid, SIGCONT);
      current = &pt->items[pt->front];

      /* Setup the alarm again */
      if (alarm(SCHED_TQ_SEC) < 0) {
        perror("alarm");
        exit(1);
      }
    }

    if (WIFSTOPPED(status)) {
      /* A child has stopped due to SIGSTOP/SIGTSTP, etc... */
      printf("Parent: Child has been stopped. Moving right along...\n");

      // remove the child and reattach it at the end of the queue
      dequeue(pt);
      enqueue(pt, *current);

      printf(" New child [ %ld ]is going to run now !\n", front(pt).procid);

      cur_pid = front(pt).procid;
      kill(cur_pid, SIGCONT);
      current = &pt->items[pt->front];

      /* Setup the alarm again */
      if (alarm(ALARM_SEC) < 0) {
        perror("alarm");
        exit(1);
      }
    }
  }
}

/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
static void install_signal_handlers(void) {
  sigset_t sigset;
  struct sigaction sa;

  sa.sa_handler = sigchld_handler;
  sa.sa_flags = SA_RESTART;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGCHLD);
  sigaddset(&sigset, SIGALRM);

  sa.sa_mask = sigset;

  // handling sigchld
  if (sigaction(SIGCHLD, &sa, NULL) < 0) {
    perror("sigaction: sigchld");
    exit(1);
  }

  // handling sigalarm
  sa.sa_handler = sigalrm_handler;

  if (sigaction(SIGALRM, &sa, NULL) < 0) {
    perror("sigaction: sigalrm");
    exit(1);
  }

  /*
   * Ignore SIGPIPE, so that write()s to pipes
   * with no reader do not result in us being killed,
   * and write() returns EPIPE instead.
   */
  if (signal(SIGPIPE, SIG_IGN) < 0) {
    perror("signal: sigpipe");
    exit(1);
  }
}

/* all prog processes do this process */
/*  prog - child process */
void doChild(char *procname) {

  char executable[] = "prog";
  char *newargv[] = {executable, NULL, NULL, NULL}; //& exexutable[0]
  char *newenviron[] = {NULL};

  printf("I am Process :%s, and PID = %ld\n", procname, (long)getpid());
  printf("About to replace myself with the executable %s...\n", executable);
  sleep(2);

  execve(executable, newargv, newenviron);

  /* execve() only returns on error */
  perror("execve");
  exit(1);
}

int main(int argc, char *argv[]) {
  int nproc;
  int i = 0;
  /*
   * For each of argv[1] to argv[argc - 1],
   * create a new child process, add it to the process list.
   */

  /* initialize the structures ,pointers */
  process = (ProcessList *)realloc(process, (argc + 1) * sizeof(ProcessList));
  pid = (pid_t *)realloc(pid, (argc + 1) * sizeof(pid_t));
  pt = newProcessQueue(argc + 1);

  /* create all the processes ,freeze all but one */
  for (i = 1; i <= argc - 1; i++) {

    pid[i - 1] = fork();

    if (pid[i - 1] == 0) {
      doChild(argv[i]);
    }

    if (pid[i - 1] < 0) {

      perror("error forking child ");
      exit(1);
    }

    process[i - 1].procid = pid[i - 1];
    strcpy(process[i - 1].procname, argv[i]);
    enqueue(pt, process[i - 1]);
    process[i - 1].TaskId = i - 1;
    printf("New process with Id:%d and pid:%ld\n", process[i - 1].TaskId,
           process[i - 1].procid);
    kill(process[i - 1].procid, SIGSTOP);
  }

  // start the first process //

  nproc = argc - 1; /* number of proccesses goes here */

  // SET UP THE ALARM
  // alarm(SCHED_TQ_SEC);

  /* Wait for all children to raise SIGSTOP before exec()ing. */
  wait_for_ready_children(nproc);

  /* Install SIGALRM and SIGCHLD handlers. */
  install_signal_handlers();

  if (nproc == 0) {
    fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
    exit(1);
  }

  /* start the first process */
  cur_pid = front(pt).procid;
  current = &pt->items[pt->front];
  kill(cur_pid, SIGCONT);
  if (alarm(SCHED_TQ_SEC) < 0) {
    perror("alarm");
    exit(1);
  }

  /* loop forever  until we exit from inside a signal handler. */
  while (pause()) {
    //  alarm (SCHED_TQ_SEC);
  }

  /* Unreachable */
  fprintf(stderr, "Internal error: Reached unreachable point\n");

  free(process);
  free(pid);
  return 1;
}
