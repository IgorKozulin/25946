#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* execvp получает команду и все её аргументы начиная с argv[1]. */
int main(int argc, char **argv)
{
    pid_t child;
    int status;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s command [arguments ...]\n", argv[0]);
        return EXIT_FAILURE;
    }
    child = fork();
    if (child == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }
    if (child == 0) {
        execvp(argv[1], &argv[1]);
        perror("execvp");
        _exit(127);
    }

    /* waitpid возвращает код состояния, из которого отдельно извлекаем exit code. */
    while (waitpid(child, &status, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            return EXIT_FAILURE;
        }
    }
    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        printf("Exit code: %d\n", code);
        return code;
    }
    if (WIFSIGNALED(status)) {
        printf("Terminated by signal: %d\n", WTERMSIG(status));
    }
    return EXIT_FAILURE;
}
