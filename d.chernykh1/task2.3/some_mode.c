#include <unistd.h>
#include <stdio.h>
void print_california_time(void);
void locateuid(void){
	FILE *file = fopen("data.txt", "r");
	if(file == NULL){
		perror("data.txt");
	}
	else{
	print_california_time();
	fclose(file);
	}
	printf("uid: %d, euid: %d\n", (int)getuid(), (int)geteuid());
}

int main(void){
	locateuid();
	if(setuid(getuid()) == -1){
	perror("setuid");
	return 1;
	}
	locateuid();
}
