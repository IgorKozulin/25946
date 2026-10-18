#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char *argv[]) {
    pid_t pid;
    int status;

    if (argc < 2) {
        printf("Usage: %s command [arguments]\n", argv[0]);
        return 1;
    }

    printf("[PARENT] Starting command: %s\n", argv[1]);

    pid = fork();

    if (pid < 0) {
        printf("Fork error\n");
        return 1;
    }

    if (pid == 0) {
        execvp(argv[1], &argv[1]);

        perror("Execvp failed");
        exit(127);
    }

    if (waitpid(pid, &status, 0) == -1) {
        printf("waitpid error\n");
        return 1;
    }

    if (WIFEXITED(status)) {
        printf("[PARENT] Command exited normally with code: %d\n", WEXITSTATUS(status));
    }

    if (WIFSIGNALED(status)) {
        int signal_number = WTERMSIG(status);

        printf("[PARENT] Command terminated by signal: %d (%s)\n", signal_number, strsignal(signal_number));
    }

    return 0;
}
