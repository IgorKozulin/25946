#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(void)
{
    FILE *child = popen("tr '[:lower:]' '[:upper:]'", "w");
    if (child == NULL) {
        perror("popen");
        return EXIT_FAILURE;
    }
    char buffer[4096];
    size_t count;
    int error = 0;
    while ((count = fread(buffer, 1, sizeof(buffer), stdin)) != 0)
        if (fwrite(buffer, 1, count, child) != count) {
            perror("write to child");
            error = 1;
            break;
        }
    if (ferror(stdin)) {
        perror("read stdin");
        error = 1;
    }
    int status = pclose(child);
    if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        error = 1;
    return error ? EXIT_FAILURE : EXIT_SUCCESS;
}
