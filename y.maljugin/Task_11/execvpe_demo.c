#include <stdio.h>
#include <stdlib.h>

int execvpe(const char *file, char *const argv[], char *const envp[]);

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s command [arguments...]\n", argv[0]);
        return EXIT_FAILURE;
    }
    char *environment[] = {"COURSE_TASK_11=custom_environment", NULL};
    execvpe(argv[1], &argv[1], environment);
    perror("execvpe");
    return EXIT_FAILURE;
}
