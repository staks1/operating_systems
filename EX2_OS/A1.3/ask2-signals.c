#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "proc-common.h"
#include "tree.h"

// here implement sigaction
void wakeMe(int sig, siginfo_t *info, void *ptr) {
  if (sig == SIGCONT) {
    printf("the process with pid : %ld is awake now !", (long)getpid());
    // HERE SHOULD WAKEUP //
  }
}

void catch_sigcont() {
  static struct sigaction _sigact;
  memset(&_sigact, 0, sizeof(_sigact));

  _sigact.sa_sigaction = wakeMe;
  _sigact.sa_flags = SA_SIGINFO; // sigaction handles instead of handler

  if (sigaction(SIGCONT, &_sigact, NULL) < 0) {
    perror("sigaction");
    return 1;
  }
}

void fork_procs(char *name, int exit_code, int number, pid_t *pid_child) {
  pid_t pid2, pid3;
  int status2, status3;

  printf("PID = %ld, name %s, starting...\n", (long)getpid(), name);

  sleep(2);
  wait_for_ready_children(number);
  raise(SIGSTOP);

  // here wake up and continue
  catch_sigcont();
  printf("PID = %ld, name %s, waking up ...\n", (long)getpid(), name);

  for (int i = 0; i < number; i++) {
    kill(pid_child[i], SIGCONT);

    pid3 = wait(&status3);
    explain_wait_status(pid3, status3);
  }

  printf("Process %s is exiting with pid:%ld\n", name, (long)getpid());
  exit(4);
}

void fork_procs2(char *name) { change_pname(name); }

void recurNode(struct tree_node *root) {

  pid_t *pid_child = (pid_t *)malloc(root->nr_children * sizeof(pid_t));
  pid_t pid;
  int status;

  change_pname(
      root->name); // change here the name -->when the process first starts

  // the root has children
  for (int i = 0; i < root->nr_children; i++) {
    pid_child[i] = fork();

    if (pid_child[i] < 0) {
      perror("error in forking children");
      exit(1);
    }
    // here we have the child (i) running
    else if (pid_child[i] == 0) {
      recurNode(&root->children[i]); // sos give the address of the next child
    }
  }

  // parent waits for all children (no zombies allowed )
  fork_procs(root->name, 1, root->nr_children, pid_child);
}

int main(int argc, char *argv[]) {
  pid_t pid0;
  int status0;
  struct tree_node *root;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
    exit(1);
  }

  /* Read tree into memory */
  root = get_tree_from_file(argv[1]);

  /* Fork root of process tree */
  pid0 = fork();

  if (pid0 < 0) {
    perror("main: fork");
    exit(1);
  }
  if (pid0 == 0) {

    print_tree(root);

    recurNode(root);
  }

  wait_for_ready_children(1);

  /* Print the process tree root at pid */
  show_pstree(pid0);

  /*here send sigcont to root */
  kill(pid0, SIGCONT);

  /* Wait for the root of the process tree to terminate */
  wait(&status0);
  explain_wait_status(pid0, status0);
  return 0;
}
