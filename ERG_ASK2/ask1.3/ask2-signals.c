#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

//here implement sigaction




void wakeMe(int sig, siginfo_t *info, void *ptr){
	if(sig==SIGCONT) {
			printf("the process with pid : %ld is awake now !",(long)getpid());
	}
}




void catch_sigcont()
{
    static struct sigaction _sigact;
    memset(&_sigact, 0, sizeof(_sigact));

    _sigact.sa_sigaction = wakeMe;
    _sigact.sa_flags = SA_SIGINFO;   //sigaction handles instead of handler

    sigaction(SIGCONT, &_sigact, NULL);
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
	pid_t pid;
	int status;
  change_pname(name);
	printf("PID = %ld, name %s, starting...\n",
			(long)getpid(), name);
			raise(SIGSTOP);


			//raise(SIGCONT);
		//	catch_sigcont();
			while ((pid = wait(&status)) > 0)
		    explain_wait_status(pid, status);
		//	exit(1);

}




void recurNode(struct tree_node *root) {

  pid_t *pid_child = (pid_t *)malloc(root->nr_children * sizeof(pid_t));
  pid_t pid;
  int status;

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
      recurNode(&root->children[i]); 				// sos give the address of the next child
    }




  }

  // parent waits for all children (no zombies allowed )
  while ((pid = wait(&status)) > 0)
    explain_wait_status(pid, status);

	//	sleep(5);


}



//Second function to wake up the processes
void recur2Node(struct tree_node *root,int exit_status) {
	int status;
	pid_t pid;

	raise(SIGCONT);
	catch_sigcont();
	while ((pid = wait(&status)) > 0)
		explain_wait_status(pid, status);
	exit(exit_status);

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
	//recur2Node(root);

	/* Wait for the root of the process tree to terminate */
	wait(&status);
	explain_wait_status(pid, status);




	return 0;
}
