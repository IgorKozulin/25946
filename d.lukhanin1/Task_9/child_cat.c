#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s FILE\n", argv[0]);
        return EXIT_FAILURE;
    }

    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }
    if (child == 0) {
        execlp("cat", "cat", "--", argv[1], (char *)NULL);
        perror("cat");
        _exit(127);
    }

    int failed = 0;
    if (printf("Parent: waiting for child.\n") < 0 || fflush(stdout) == EOF) {
        perror("stdout");
        failed = 1;
    }
    int status;
    while (waitpid(child, &status, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            return EXIT_FAILURE;
        }
    }
    if (printf("Parent: child finished.\n") < 0 || fflush(stdout) == EOF) {
        perror("stdout");
        failed = 1;
    }
    return !failed && WIFEXITED(status) && WEXITSTATUS(status) == 0
        ? EXIT_SUCCESS : EXIT_FAILURE;
}
