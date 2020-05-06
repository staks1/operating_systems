#include <stdio.h>
#include <stdlib.h>
#include "tree.h"
#include <sys/types.h>
#include <unistd.h>

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3


//create the functions
void fork_procs(char *name, int exit_code) {
  change_pname(name);
	sleep(3);   //maybe
  printf("%s: Exiting...\n", name);
  exit(exit_code);
}

void fork_procs2(char *name) {
  change_pname(name);  //maybe
	sleep(3);
}



void recurNode(struct tree_node* root){

	pid_t* pid_child=(pid_t *)malloc(root->nr_children * sizeof(pid_t));
	pid_t pid ;
	int status;


//the root has children
for (int i=0;i<root->nr_children; i++){
	pid_child[i]=fork();


	if(pid_child[i]<0){
		perror("error in forking children");
		exit(1);
	}

	//here we have the child (i) running
	else if(pid_child[i]==0){
			fork_procs2(&root->children[i].name); //maybe
			recurNode(&root->children[i]) ;
			//sos give the address of the next child
   }
 }

// if(pid_child[i]>0){ wait for all children (no zombies allowed )
    while((pid=wait(&status))>0)
       explain_wait_status(pid, status);


	//no children -->that means (maybe) that it is a leaf
			fork_procs(root->name,1);
			//sleep(1);
}






int main(int argc, char *argv[])
{
	struct tree_node *root;
	//int i=0;
	//pid_t* pid_all ;
	pid_t pid0;
	int status0;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
		exit(1);
	}

  root = get_tree_from_file(argv[1]);

	pid0=fork();

	if(pid0<0){
		perror("error in forking the recursive function process");

	}if(pid0==0){
		print_tree(root);
		recurNode(root);
		//show_pstree(pid0);
}

	sleep(4);
	//parent process waits for the function processe
show_pstree(pid0);
	/* for ask2-{fork, tree} */
	//sleep(20);

	/* Print the process tree root at pid */


	/* Wait for the root of the process tree to terminate */
	pid0 = wait(&status0);
	explain_wait_status(pid0, status0);

	return 0;
}
