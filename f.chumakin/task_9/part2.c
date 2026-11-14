#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    pid_t pid;
    int status;

    if (argc != 2) {
        printf("Usage: %s file\n", argv[0]);
        return 1;
    }

    pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        printf("[CHILD] pid=%d\n", (int)getpid());

        execlp("cat", "cat", argv[1], (char *)NULL);

        perror("execlp");
        exit(1);
    }

    printf("[PARENT] pid=%d child=%d\n",
           (int)getpid(), (int)pid);

    printf("[PARENT] Parent is working\n");

    waitpid(pid, &status, 0);

    printf("[PARENT] Child finished\n");

    if (WIFEXITED(status)) {
        printf("[PARENT] Child exit status = %d\n",
               WEXITSTATUS(status));
    }

    printf("[PARENT] Last parent line\n");

    return 0;
}
