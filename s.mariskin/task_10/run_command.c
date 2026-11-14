#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int status;
    pid_t child;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s COMMAND [ARG ...]\n", argv[0]);
        return EXIT_FAILURE;
    }
    child = fork();
    if (child == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }
    if (child == 0) {
        /* Передаем команде исходные аргументы без имени нашей программы */
        execvp(argv[1], &argv[1]);
        {
            int code = errno == ENOENT ? 127 : 126;
            perror("execvp");
            _exit(code);
        }
    }

    while (waitpid(child, &status, 0) == -1) {
        if (errno == EINTR) continue;
        perror("waitpid");
        return EXIT_FAILURE;
    }
    /* Статус waitpid содержит причину завершения, а не только код выхода */
    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        printf("[PARENT] Command exited normally with code: %d\n", code);
        return code;
    }
    if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        printf("[PARENT] Command terminated by signal: %d (%s)\n", sig, strsignal(sig));
        return 128 + sig;
    }
    return EXIT_FAILURE;
}
