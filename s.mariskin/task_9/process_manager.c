#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int no_wait = argc == 3 && strcmp(argv[1], "--no-wait") == 0;
    int status;
    pid_t child;
    const char *path;

    if (argc != 2 && !no_wait) {
        fprintf(stderr, "Usage: %s [--no-wait] FILE\n", argv[0]);
        return EXIT_FAILURE;
    }
    path = argv[no_wait ? 2 : 1];
    child = fork();
    if (child == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }
    if (child == 0) {
        /* При успехе exec заменяет код дочернего процесса программой cat */
        execlp("cat", "cat", "--", path, (char *)NULL);
        perror("execlp cat");
        _exit(127);
    }

    printf("[PARENT] Child PID: %ld\n", (long)child);
    fflush(stdout);
    if (no_wait) {
        puts("[PARENT] Last line without waiting");
        return EXIT_SUCCESS;
    }

    /* После waitpid дочерний процесс уже не может продолжать вывод */
    while (waitpid(child, &status, 0) == -1) {
        if (errno == EINTR) continue;
        perror("waitpid");
        return EXIT_FAILURE;
    }
    if (WIFEXITED(status)) {
        printf("[PARENT] Child exit code: %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("[PARENT] Child terminated by signal: %d\n", WTERMSIG(status));
    }
    puts("[PARENT] Child has finished. This is the last line.");
    return WIFEXITED(status) ? WEXITSTATUS(status) : EXIT_FAILURE;
}
