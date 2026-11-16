#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    pid_t child1;
    pid_t child2;
    int status;

    if (argc != 2) {
        printf("Usage: %s filename\n", argv[0]);
        return 1;
    }

    child1 = fork();

    if (child1 < 0) {
        printf("Fork error\n");
        return 1;
    }

    if (child1 == 0) {
        execlp("cat", "cat", argv[1], NULL);

        perror("execlp");
        exit(1);
    }

    waitpid(child1, &status, 0);

    child2 = fork();

    if (child2 < 0) {
        printf("Fork error\n");
        return 1;
    }

    if (child2 == 0) {
        printf("[CHILD 2] My PID: %d\n", (int)getpid());
        exit(0);
    }

    waitpid(child2, &status, 0);

    printf("[PARENT] My PID: %d\n", (int)getpid());

    return 0;
}
