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
  change_pname(name);
  printf("%s: Exiting...\n", name);
  exit(exit_code);
}

int main(void) {
  pid_t pid2, pid3, pid4, pid5;
  int status, status2;

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
        sleep(SLEEP_PROC_SEC);
        fork_procs("D", 13);
      }

      // B is waiting
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
      sleep(SLEEP_PROC_SEC + 1); // (+1 ) set up a little bigger time for C so D
                                 // finishes first ( for better visualization)
      fork_procs("C", 17);
    }

    // A should wait for both children B and C to finish
    for (int i = 0; i < 2; i++) {
      pid_all[i] = wait(&status_all[i]);
    }

    explain_wait_status(pid_all[0], status_all[0]);
    explain_wait_status(pid_all[1], status_all[1]);

    fork_procs("A", 16);
  }

  /* for ask2-{fork, tree} */
  sleep(SLEEP_TREE_SEC);

  /* Print the process tree root at pid */
  show_pstree(pid2);

  /* Wait for the root of the process tree to terminate */
  pid2 = wait(&status);
  explain_wait_status(pid2, status);

  return 0;
}
