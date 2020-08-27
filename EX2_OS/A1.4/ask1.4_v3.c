#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "proc-common.h"
#include "tree.h"

int calculate(char *name, int val1, int val2) {
  if (strcmp(name, "+") == 0)
    return val1 + val2;
  else if (strcmp(name, "*") == 0)
    return val1 * val2;
}

void recurNode(struct tree_node *root, int pfd) {

  pid_t temp_pid;
  int status, pfd_c1[2], pfd_c2[2], op1, op2, temp, i;
  char *ptr;

  change_pname(root->name);

  // creating pipes//
  if (pipe(pfd_c1) < 0) {
    perror("error creating pipe\n");
    exit(1);
  }
  if (pipe(pfd_c2) < 0) {
    perror("error creating pipe\n");
    exit(1);
  }

  // passing pipes//
  for (i = 0; i < root->nr_children; i++) {
    temp_pid = fork();
    if (temp_pid < 0) {
      perror("error creating fork\n");
    } else if (temp_pid == 0) {
      if (i == 0) {
        close(pfd_c1[0]);
        recurNode(root->children + i, pfd_c1[1]);
      } else {
        close(pfd_c2[0]);
        recurNode(root->children + i, pfd_c2[1]);
      }
    }
  }

  // for leaf nodes //
  if (root->nr_children == 0) {
    // read name and convert to number//
    temp = strtol(root->name, &ptr, 10);
    if (write(pfd, &temp, sizeof(temp)) != sizeof(temp)) {

      perror("error writing leaf nodes\n");
    }
  }

  // for parents waiting to calculate//
  else {
    while (temp_pid = wait(&status) > 0)
      explain_wait_status(temp_pid, status);
    // close the writing end of child 1//
    close(pfd_c1[1]);

    if (read(pfd_c1[0], &op1, sizeof(op1)) != sizeof(op1)) {
      perror("error reading from child 1\n");
    }
    // close the writing end of child 2//
    close(pfd_c2[1]);

    if (read(pfd_c2[0], &op2, sizeof(op2)) != sizeof(op2)) {
      perror("error reading from child 2\n");
    }
    // calculate answer//
    temp = calculate(root->name, op1, op2);

    // write the answer//
    write(pfd, &temp, sizeof(temp));
    printf("The answer of << %d %s %d >> is %d\n", op1, root->name, op2, temp);
  }

  printf("%s :exiting\n", root->name);
  exit(1);
}

////////---MAIN FUNCTION---/////////
int main(int argc, char *argv[]) {
  struct tree_node *root;

  pid_t pid0;
  int status0;
  int pfd[2], result;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
    exit(1);
  }

  root = get_tree_from_file(argv[1]);
  print_tree(root);

  if (pipe(pfd) < 0) {
    perror("pipe");
    exit(1);
  }

  pid0 = fork();

  if (pid0 < 0) {
    perror("error in forking the recursive function's process\n");
  }
  if (pid0 == 0) {
    close(pfd[0]);
    recurNode(root, pfd[1]);
    exit(1);
  }

  // parent reads the result//
  close(pfd[1]);
  read(pfd[0], &result, sizeof(result));
  printf("result is :%d\n", result);

  // parent process waits for the function processe

  /* Wait for the root of the process tree to terminate */
  pid0 = wait(&status0);
  explain_wait_status(pid0, status0);

  return 0;
}
