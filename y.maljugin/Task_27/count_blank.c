#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s text-file\n", argv[0]);
        return EXIT_FAILURE;
    }
    FILE *input = fopen(argv[1], "r");
    if (input == NULL) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }
    /* Feed one newline to wc for every empty line in the input. */
    FILE *counter = popen("wc -l", "w");
    if (counter == NULL) {
        perror("popen wc");
        fclose(input);
        return EXIT_FAILURE;
    }

    int ch, has_content = 0, error = 0;
    while ((ch = fgetc(input)) != EOF) {
        if (ch == '\n') {
            if (!has_content && fputc('\n', counter) == EOF) {
                perror("write wc");
                error = 1;
                break;
            }
            has_content = 0;
        } else if (ch != '\r') {
            has_content = 1;
        }
    }
    if (ferror(input)) {
        perror("read input");
        error = 1;
    }
    fclose(input);
    int status = pclose(counter);
    if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        error = 1;
    return error ? EXIT_FAILURE : EXIT_SUCCESS;
}
