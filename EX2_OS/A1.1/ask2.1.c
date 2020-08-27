#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "proc-common.h"

#define SLEEP_PROC_SEC 10
#define SLEEP_TREE_SEC 3

/*
 * Create this process tree:
 * A-+-B---D
 *   `-C
 */

void fork_procs(char *name, int exit_code) {
  sleep(7);
  printf("%s: Exiting...\n", name);
  exit(exit_code);
}

int main(void) {
  pid_t pid2, pid3, pid4, pid5;
  int status, status2;
  int i = 0;

  pid_t pid_all[2];
  int status_all[2];

  pid2 = fork(); // create A
  if (pid2 < 0) {
    perror("A: fork");
    exit(1);
  }

  if (pid2 == 0) { // A is running
    pid3 = fork(); // create B

    if (pid3 < 0) {
      perror("B: fork");
      exit(1);
    }

    if (pid3 == 0) {
      pid4 = fork(); // create D

      if (pid4 < 0) {
        perror("D: fork");
        exit(1);
      }
      if (pid4 == 0) { // D IS running
        change_pname("D");
        printf("Starting ...D\n");

        fork_procs("D", 13);
      }

      // B is waiting
      change_pname("B");
      printf("Starting ...B\n");
      pid4 = wait(&status2);

      explain_wait_status(pid4, status2);

      fork_procs("B", 19);
    }

    pid5 = fork(); // create C
    if (pid5 < 0) {
      perror("C: fork");
      exit(1);
    }

    if (pid5 == 0) {
      sleep(2);
      change_pname("C");
      printf("Starting ...C\n");

      fork_procs("C", 17);
    }

    // A should wait for both children B and C to finish
    change_pname("A");
    printf("Starting ...A\n");
    while ((pid_all[i] = wait(&status_all[i])) > 0) {
      explain_wait_status(pid_all[i], status_all[i]);
      i++;
    }

    fork_procs("A", 16);
  }

  /* for ask2-{fork, tree} */

  sleep(2);
  /* Print the process tree root at pid */
  show_pstree(pid2);

  /* Wait for the root of the process tree to terminate */
  pid2 = wait(&status);
  explain_wait_status(pid2, status);

  return 0;
}
