#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* second function to implement write */
void write_file(int fd, char *buff, int len) {
	int idx = 0;
	int wcnt;

	while (idx < len) {
		wcnt = write(fd, buff + idx, len - idx);
		if (wcnt == -1) {
			perror("error in writing");
			exit(1);
		}
		idx += wcnt;
	}
}

/* first function implements read and calls write */
void doWrite(int fd1, int fd2, char *buff) {
	int rcnt;
	int total = 0, len = 0;

	for (;;) {

		rcnt = read(fd1, buff, sizeof(buff) - 1);
		total = rcnt + total;
		if (rcnt == 0)
			break;
		if (rcnt == -1) {
			perror("problem in read");
			exit(1);
		}

		buff[rcnt] = '\0';
		len = strlen(buff);
		write_file(fd2, buff, len);
	}
}

int main(int argc, char *argv[]) {
	int check = 0, target;
	char buff[1024];
	char buff2[1024];
	int f1, f2, f3;

	// case1:not enough /more files tha appropriate
	if (argc < 3 || argc > 4) {
		printf("Error:Please pass 2 files for input and no more than 1 file for "
					"output\n");
		exit(1);
	}

 // case 2 :file inputs not exist
	f1 = open(argv[1], O_RDWR, 0777);
	if (f1 == -1) {
		perror("Error:opening file 1 ,maybe it does not exist\n");
		exit(1);
	}
	f2 = open(argv[2], O_RDWR, 0777);
	if (f1 == -1) {
		perror("Error:opening file 2 ,maybe it does not exist\n");
		exit(1);
	}
	// case 1 b : file 3 is the same as file 1 or file 2


	// creating file 3
	if (argc == 4) {
		if ((strcmp(argv[3], argv[1]) == 0
						|| strcmp(argv[3], argv[2]) == 0)) {
		//printf("output file can not be the same as input file");
		//exit(1);
		f3 = open("tmp", O_RDWR|O_TRUNC|O_CREAT, 0777);
		if(f3 == -1){
			perror("gt re malaka:");
			return 1;
		}
		check = 1;
		if(!(strcmp(argv[3], argv[1]))) target = f1;
		else target = f2;
		}
	 	else{

		 	f3 = open(argv[3], O_CREAT | O_RDWR | O_APPEND | O_TRUNC, 0644);
			if (f3 == -1) {
				perror("error opening output file");
				exit(1);
			}
		}
	}
	else if (argc < 4) {
	 	f3 = open("fconc.out",  O_TRUNC|O_CREAT | O_RDWR | O_APPEND, 0644);
	 	if (f3 == -1) {
			perror("error opening output file");
			exit(1);
	 	}
 	}

	// read and write file 1 ,file 2 to file 3 and close files
	doWrite(f1, f3, buff);
	doWrite(f2, f3, buff2);
	if(check == 1){
		// lseek einai syscall poy soy dinei thn dynatothta na alla3eis ton pointer toy arxeioy
		if(lseek(f3, 0, SEEK_SET) < 0) {
			perror("it failed to move the pointer of the tmp file");
			return 1;
		}
		if(lseek(target, 0, SEEK_SET) < 0) {
			perror("it failed to move the pointer of the target file");
			return 1;
		}

		doWrite(f3, target, buff);
	}
	close(f1);
	close(f2);
	close(f3);
	if(check){
		if(unlink("tmp") == -1){
			perror("Failed to delete tmp file used\
				for writing in one of the input files\n");
			return 1;
		}
	}
	return 0;
}
