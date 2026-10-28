#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* ---------- Первый ребёнок: cat файла ---------- */
    pid_t pid1 = fork();

    if (pid1 < 0) {
        perror("fork1");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0) {
        printf("[CHILD1] I am the first child. My PID: %d\n", getpid());
        fflush(stdout);

        execlp("cat", "cat", argv[1], (char *)NULL);

        perror("execlp");
        exit(EXIT_FAILURE);
    }

    /* ---------- Второй ребёнок: просто строка ---------- */
    pid_t pid2 = fork();

    if (pid2 < 0) {
        perror("fork2");
        exit(EXIT_FAILURE);
    }

    if (pid2 == 0) {
        printf("[CHILD2] I am the second child. My PID: %d\n", getpid());
        printf("[CHILD2] I just print this line and exit.\n");
        fflush(stdout);

        exit(EXIT_SUCCESS);
    }

    /* ---------- Родитель ---------- */
    int status;
    int i;

    printf("[PARENT] Starting program. My PID: %d\n", getpid());
    printf("[PARENT] Child1 PID: %d, Child2 PID: %d\n", pid1, pid2);
    printf("[PARENT] Printing some text while children are working...\n");
    printf("[PARENT] Waiting for children to finish...\n");
    fflush(stdout);

    /* Ждём ОБОИХ детей */
    for (i = 0; i < 2; i++) {
        pid_t finished = waitpid(-1, &status, 0);   /* -1 = любой ребёнок */

        if (finished == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(status)) {
            printf("[PARENT] Child with PID %d exited with status: %d\n",
                   finished, WEXITSTATUS(status));
        } else {
            printf("[PARENT] Child with PID %d was terminated abnormally\n",
                   finished);
        }
    }

    printf("[PARENT] All children have finished. "
           "This is the last line printed by parent.\n");

    return 0;
}
