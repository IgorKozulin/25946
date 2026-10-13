#include <unistd.h>
#include <stdio.h>

void locateuid(void){
	FILE *file = fopen("data.txt", "r");
	if(file == NULL){
		perror("data.txt");
	}
	else{
	printf("file was open\n");
	fclose(file);
	}
	printf("uid: %d, euid: %d\n", (int)getuid(), (int)geteuid());
}

int main(void){
	locateuid();
	if(setuid(getuid()) == -1){
	perror("setuid");
	}
	locateuid();
}
