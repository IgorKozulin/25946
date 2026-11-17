#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* Дочерний процесс печатает файл через cat; родитель ждёт именно его PID. */
int main(int argc, char **argv)
{
    pid_t child;
    int status;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return EXIT_FAILURE;
    }

    child = fork();
    if (child == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }
    if (child == 0) {
        execlp("cat", "cat", argv[1], (char *)NULL);
        perror("execlp cat");
        _exit(127); /* exec возвращается только при ошибке. */
    }

    printf("Parent: child PID = %ld\n", (long)child);
    puts("Parent: I am working while cat prints the file.");
    fflush(stdout);
    /* Последнее сообщение печатаем только после завершения cat. */
    while (waitpid(child, &status, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            return EXIT_FAILURE;
        }
    }
    puts("Parent: child has finished; this is my last line.");
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return EXIT_FAILURE;
}
