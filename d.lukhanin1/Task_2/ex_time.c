#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    time_t now = time(NULL);
    if (now == (time_t)-1) {
        perror("time");
        return EXIT_FAILURE;
    }

    /* PST is UTC-8 all year; no daylight saving adjustment. */
    if (setenv("TZ", "PST8", 1) != 0) {
        perror("setenv");
        return EXIT_FAILURE;
    }
    tzset();

    struct tm *california = localtime(&now);
    if (california == NULL) {
        fprintf(stderr, "localtime failed\n");
        return EXIT_FAILURE;
    }

    char buffer[64];
    if (strftime(buffer, sizeof buffer, "%Y-%m-%d %H:%M:%S PST", california) == 0) {
        fprintf(stderr, "strftime failed\n");
        return EXIT_FAILURE;
    }

    puts(buffer);
    return EXIT_SUCCESS;
}
