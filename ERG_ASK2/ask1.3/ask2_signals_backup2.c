#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"



void wakeMe(char* name,int pid){
	kill(pid,SIGCONT);

}

void fork_procs(char* name ,int exit_status)
{
//	printf("PID = %ld, name %s, starting...\n",
	//		(long)getpid(), name);
	change_pname(name);

	 printf("the process with PID = %ld, name = %s is goind to sleep\n",
 		(long)getpid(),name);
		raise(SIGSTOP);  //stop self till new signal is received

	//here the process is awakened
	printf("PID = %ld, name = %s is awake\n",
		(long)getpid(),name);

	exit(0);
}



void fork_procs2(char* name) {
  change_pname(name);
	printf("PID = %ld, name %s, starting...\n",
			(long)getpid(), name);
}




void recurNode(struct tree_node *root) {

  pid_t *pid_child = (pid_t *)malloc(root->nr_children * sizeof(pid_t));
  pid_t pid;
  int status;



//  change_pname(
  //    root->name); // change here the name -->when the process first starts

fork_procs2(&root->name);

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
  while ((pid = wait(&status)) > 0)
    explain_wait_status(pid, status);

	//	printf("\n the process with pid :%ld is going to sleep :",(long)getpid());
		raise(SIGSTOP);  //maybe here the parent raises stop after it is sure that children are stopped

  // no children -->that means (maybe) that it is a leaf

}







/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 *
 * How to wait for the process tree to be ready?
 * In ask2-{fork, tree}:
 *      wait for a few seconds, hope for the best.
 * In ask2-signals:
 *      use wait_for_ready_children() to wait until
 *      the first process raises SIGSTOP.
*/

int main(int argc, char *argv[])
{
	pid_t pid;
	int status;
	struct tree_node *root;

	if (argc < 2){
		fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
		exit(1);
	}

	/* Read tree into memory */
	root = get_tree_from_file(argv[1]);

	/* Fork root of process tree */
	pid = fork();
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {

	    print_tree(root);

	    recurNode(root);
	}

	/*
	 * Father
	 */
	/* for ask2-signals */
	wait_for_ready_children(1);


	/* Print the process tree root at pid */
	show_pstree(pid);

	/* for ask2-signals */
	kill(pid, SIGCONT);

	/* Wait for the root of the process tree to terminate */
	wait(&status);
	explain_wait_status(pid, status);




	return 0;
}
