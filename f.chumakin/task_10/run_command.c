#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char *argv[])
{
    pid_t pid;
    int status;

    if (argc < 2) {
        printf("Usage: %s command [args...]\n", argv[0]);
        return 1;
    }

    printf("[PARENT] Starting command: %s\n", argv[1]);
    fflush(stdout);

    pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        execvp(argv[1], &argv[1]);

        perror("execvp");
        _exit(127);
    }

    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        printf("[PARENT] Command exited with code: %d\n",
               WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("[PARENT] Command terminated by signal: %d (%s)\n",
               WTERMSIG(status),
               strsignal(WTERMSIG(status)));
    }

    return 0;
}
