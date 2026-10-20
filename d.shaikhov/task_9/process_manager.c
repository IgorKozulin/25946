#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    pid_t pid;
    int status;

    if (argc != 2) {
        printf("Usage: %s filename\n", argv[0]);
        return 1;
    }

    printf("[PARENT] Starting program. My PID: %d\n", (int)getpid());
    fflush(stdout);

    pid = fork();

    if (pid < 0) {
        printf("Fork error\n");
        return 1;
    }

    if (pid == 0) {
        printf("[CHILD] I am the child process. My PID: %d\n", (int)getpid());

        fflush(stdout);

        execlp("cat", "cat", argv[1], NULL);

        perror("execlp");
        exit(EXIT_FAILURE);
    }

    printf("[PARENT] I am the parent. Child PID: %d\n", (int)pid);

    printf("[PARENT] Printing some text while child is working...\n");

    printf("[PARENT] Waiting for child to finish...\n");

    if (waitpid(pid, &status, 0) == -1) {
        printf("waitpid error\n");
        return 1;
    }

    if (WIFEXITED(status)) {
        printf("[PARENT] Child exited with status: %d\n", WEXITSTATUS(status));
    }

    printf("[PARENT] Child has finished. This is the last line printed by parent.\n");

    return 0;
}
