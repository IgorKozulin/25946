#define _POSIX_C_SOURCE 200809L
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void)
{
    FILE *streams[2];
    /* head stops after 100 numbers, so sort sees EOF without a close/read race. */
    if (p2open("head -n 100 | sort -n", streams) == -1) {
        perror("p2open");
        return EXIT_FAILURE;
    }

    srand((unsigned int)time(NULL));
    int error = 0;
    for (int i = 0; i < 100; ++i)
        if (fprintf(streams[0], "%d\n", rand() % 100) < 0) {
            perror("write to sort");
            error = 1;
            break;
        }
    if (fflush(streams[0]) == EOF) {
        perror("flush to sort");
        error = 1;
    }

    char line[64];
    int count = 0;
    while (!error && fgets(line, sizeof(line), streams[1]) != NULL) {
        char *end;
        long number = strtol(line, &end, 10);
        if (end == line || number < 0 || number > 99) {
            fprintf(stderr, "Invalid number from sort.\n");
            error = 1;
            break;
        }
        printf("%2ld%c", number, (++count % 10 == 0) ? '\n' : ' ');
    }
    if (ferror(streams[1]) || count != 100)
        error = 1;
    if (p2close(streams) == -1)
        error = 1;
    return error ? EXIT_FAILURE : EXIT_SUCCESS;
}
