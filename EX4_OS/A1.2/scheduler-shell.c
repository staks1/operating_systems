#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"
#include "request.h"

/* Compile-time parameters. */
#define SCHED_TQ_SEC 10               /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */
#define SHELL_EXECUTABLE_NAME "shell" /* executable for shell */
#define SLEEP_SEC 1
#define ALARM_SEC 2

/* The function of the prog processses-children */
void doChild(char *procname) {
  char *newargv[] = {procname, NULL, NULL, NULL}; //& exexutable[0]
  char *newenviron[] = {NULL};

  printf("I am Process :%s, and PID = %ld\n", procname, (long)getpid());
  printf("About to replace myself with the executable %s...\n", procname);
  sleep(2);

  execve(procname, newargv, newenviron);

  /* execve() only returns on error */
  perror("execve");
  exit(1);
}

/* queue -processes implementation*/
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

  printf("Inserting process %s  with pid %ld  and Id :%d at the end of the "
         "queue \n",
         x.procname, x.procid, x.TaskId);

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

/*function to remove item from middle of queue */
struct queue *removeFromMiddle(struct queue *pt, pid_t pid) {
  ProcessList *cu;

  if (front(pt).procid == pid) {
    cu = &(pt->items[pt->front]);

    dequeue(pt);
    printf("The removed item is the process with id :%ld and name :%s \n",
           cu->procid, cu->procname);
    return pt;
  } else {
    while (front(pt).procid != pid && !isEmpty(pt)) {
      cu = &(pt->items[pt->front]);
      dequeue(pt);
      enqueue(pt, *cu);
    }

    // remove the actual item we want
    cu = &(pt->items[pt->front]);
    dequeue(pt);

    printf("The removed item is the process with id :%ld and name :%s \n",
           cu->procid, cu->procname);
    return pt;
  }
}

int newId = 0;

/* function to print list (maybe will use !) */

void printQueue(struct queue *pt) {
  // ProcessList r=front(pt);
  bool found = false;
  int f = pt->front;
  int r = pt->rear;
  int s = sizeof(pt->items);
  int i = 0;

  printf("Number of processes :%d\n", size(pt));
  // access the items array of the queue to get the remaining processes running
  printf("--------------PROCESSES----------------------------------------------"
         "----\n");
  for (i = f; i <= r; i++) { // i< pt->size

    printf("Process with name : %s , Id: %d ,pid: %ld\n",
           (pt->items[i]).procname, (pt->items[i]).TaskId,
           (pt->items[i]).procid);
    if (i == r) {
      found = true;
      break;
    }
  }

  printf("----Runnin Process name:%s "
         "-------------------------------------------\n",
         front(pt).procname);
  printf("--------------^PROCESSES^--------------------------------------------"
         "----\n");
}

/* finished Queue implementation  */

/*initializing global pointers/variables */
pid_t cur_pid = 0;
ProcessList current;
struct queue *pt = NULL;
ProcessList *process = NULL;
pid_t *pid = NULL;
pid_t shelpid;
int nproc;

pid_t pidnew;

// define boolean flag to check if process was killed from user (through the
// shell)
bool userKilled = false;

/* The Functions of the Shell */

/* Print a list of all tasks currently being scheduled.  */
static void sched_print_tasks(void) { printQueue(pt); }

/* Send SIGKILL to a task determined by the value of its
 * scheduler-specific id.
 */
static int sched_kill_task_by_id(int id) {
  pid_t tempid;
  // assert(0 && "Please fill me!");
  bool exists = false;
  printf("The process with id :%d is going to be terminated unless it is "
         "already terminated !\n",
         id);

  int f = pt->front;
  int r = pt->rear;
  int s = sizeof(pt->items);
  int i = 0;
  for (i = f; i <= r; i++) {
    if ((pt->items[i]).TaskId == id) {
      tempid = pt->items[i].procid;
      exists = true;
    }
  }

  if (exists == false) {
    printf("Whoa there !This process does not exist !");
  } else {

    kill(tempid, SIGTERM); // maybe sigterm or sigkill
    userKilled = true;
  }

  // return -ENOSYS;
}

/* Create a new task.  */
static void sched_create_task(char *executable) {
  ProcessList newprocess;

  pidnew = fork();

  if (pidnew < 0) {
    perror("error forking child ");
    exit(1);
  }

  else if (pidnew == 0) {
    doChild(executable);
  } else {

    // here define the new process and add it to the queue
    printf("Child process : %s with pid: %ld and Id:%d was created\n",
           executable, pidnew, newId);
    newprocess.procid = pidnew;
    newprocess.TaskId = newId;
    strcpy(newprocess.procname, executable);

    // pt=resize(pt);
    enqueue(pt, newprocess);

    printf("stopping...process\n");
    kill(newprocess.procid, SIGSTOP);

    newId = newId + 1;
  }
}

/* Process requests by the shell.  */
static int process_request(struct request_struct *rq) {
  switch (rq->request_no) {
  case REQ_PRINT_TASKS:
    sched_print_tasks();
    return 0;

  case REQ_KILL_TASK:
    return sched_kill_task_by_id(rq->task_arg);

  case REQ_EXEC_TASK:
    sched_create_task(rq->exec_task_arg);
    return 0;

  default:
    return -ENOSYS;
  }
}

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
  printf("This child [ %ld ] must now be put on hold (back of the queue).\n",
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
      if (!isEmpty(pt) && userKilled == false) {
        dequeue(pt);
      }

      else if (!isEmpty(pt) && userKilled == true) {
        pt = removeFromMiddle(pt, p);
        userKilled = false;

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
      current = front(pt);

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
      enqueue(pt, current);

      printf(" New child [ %ld ] is going to run now !\n", front(pt).procid);

      cur_pid = front(pt).procid;
      kill(cur_pid, SIGCONT);
      current = front(pt);

      /* Setup the alarm again */
      if (alarm(ALARM_SEC) < 0) {
        perror("alarm");
        exit(1);
      }
    }
  }
}

/* Disable delivery of SIGALRM and SIGCHLD. */
static void signals_disable(void) {
  sigset_t sigset;

  sigemptyset(&sigset);
  sigaddset(&sigset, SIGALRM);
  sigaddset(&sigset, SIGCHLD);
  if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
    perror("signals_disable: sigprocmask");
    exit(1);
  }
}

/* Enable delivery of SIGALRM and SIGCHLD.  */
static void signals_enable(void) {
  sigset_t sigset;

  sigemptyset(&sigset);
  sigaddset(&sigset, SIGALRM);
  sigaddset(&sigset, SIGCHLD);
  if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
    perror("signals_enable: sigprocmask");
    exit(1);
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

  if (sigaction(SIGCHLD, &sa, NULL) < 0) {
    perror("sigaction: sigchld");
    exit(1);
  }

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

/* do the shell operations */
static void do_shell(char *executable, int wfd, int rfd) {
  char arg1[10], arg2[10];
  char *newargv[] = {executable, NULL, NULL, NULL};
  char *newenviron[] = {NULL};

  // write/read file descriptors
  sprintf(arg1, "%05d", wfd);
  sprintf(arg2, "%05d", rfd);

  newargv[1] = arg1;
  newargv[2] = arg2;

  raise(SIGSTOP);
  execve(executable, newargv, newenviron);

  /* execve() only returns on error */
  perror("scheduler: child: execve");
  exit(1);
}

/* creating the SHELL */
/* Create a new shell task.
 *
 * The shell gets special treatment:
 * two pipes are created for communication and passed
 * as command-line arguments to the executable.
 */
static pid_t sched_create_shell(char *executable, int *request_fd,
                                int *return_fd) {
  pid_t p;
  int pfds_rq[2], pfds_ret[2];

  if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
    perror("pipe");
    exit(1);
  }

  p = fork();
  if (p < 0) {
    perror("scheduler: fork");
    exit(1);
  }

  if (p == 0) {
    /* Child */
    close(pfds_rq[0]);
    close(pfds_ret[1]);
    do_shell(executable, pfds_rq[1], pfds_ret[0]);
    // assert(0);
  }

  /* Parent */
  close(pfds_rq[1]);
  close(pfds_ret[0]);
  *request_fd = pfds_rq[0];
  *return_fd = pfds_ret[1];
  // ADD THE SHELL
  if (p != 0) {
    return p;
  }
}

static void shell_request_loop(int request_fd, int return_fd) {
  int ret;
  struct request_struct rq;

  /*
   * Keep receiving requests from the shell.
   */
  for (;;) {
    if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
      perror("scheduler: read from shell");
      fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
      break;
    }

    signals_disable();
    ret = process_request(&rq);
    signals_enable();

    if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
      perror("scheduler: write to shell");
      fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
      break;
    }
  }
}

int main(int argc, char *argv[]) {

  /* Two file descriptors for communication with the shell */
  static int request_fd, return_fd;
  int i = 0;

  /* create the child processes */
  /* initialize the structures ,pointers */
  process = (ProcessList *)realloc(process, (argc) * sizeof(ProcessList));
  pid = (pid_t *)realloc(pid, (argc) * sizeof(pid_t));
  //  pt = newProcessQueue(argc); //correct

  pt = newProcessQueue(100);

  /* Create the shell. */
  shelpid = sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);
  /*  add the shell to the scheduler's tasks */
  process[0].procid = shelpid;
  // set the id for the shell -> 0
  process[0].TaskId = 0;
  process[0].next = &process[1];
  strcpy(process[0].procname, "shell");
  enqueue(pt, process[0]);
  kill(process[0].procid, SIGSTOP);

  /* create all the processes ,freeze all but one */
  for (i = 1; i <= argc - 1; i++) {

    pid[i] = fork();

    if (pid[i] == 0) {
      doChild(argv[i]);
    }

    if (pid[i] < 0) {

      perror("error forking child ");
      exit(1);
    }

    // add pointers to create cyclic list
    if (i != argc - 1) {
      process[i].next = &process[i + 1];
    }

    // the last points to the front
    if (i == argc - 1) {
      process[i].next = NULL;
    }

    process[i].procid = pid[i];

    // use the  "i" as task id ->used later to kill process
    process[i].TaskId = i;
    strcpy(process[i].procname, argv[i]);
    enqueue(pt, process[i]);
    kill(process[i].procid, SIGSTOP);
  }

  /*
   * For each of argv[1] to argv[argc - 1],
   * create a new child process, add it to the process list.
   */

  nproc = argc; /* number of proccesses goes here (argc -1
                                                          ,+1 for the shell
                   process)*/

  newId = argc; // second variable to use for creating Id for new processes

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
  current = front(pt);

  kill(cur_pid, SIGCONT);
  if (alarm(SCHED_TQ_SEC) < 0) {
    perror("alarm");
    exit(1);
  }

  shell_request_loop(request_fd, return_fd);

  /* Now that the shell is gone, just loop forever
   * until we exit from inside a signal handler.
   */
  while (pause())
    ;

  /* Unreachable */
  fprintf(stderr, "Internal error: Reached unreachable point\n");
  return 1;
}
