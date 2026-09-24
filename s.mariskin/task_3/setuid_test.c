#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

static void print_ids(const char *stage)
{
    printf("%s\n", stage);
    printf("real_uid=%lu\n", (unsigned long)getuid());
    printf("effective_uid=%lu\n", (unsigned long)geteuid());
}

static int try_open_data(const char *stage)
{
    FILE *data = fopen("data.txt", "r");

    if (data == NULL) {
        fprintf(stderr, "%s: ", stage);
        perror("fopen data.txt");
        return 0;
    }

    printf("%s: data.txt opened successfully\n", stage);
    if (fclose(data) == EOF) {
        perror("fclose data.txt");
        return -1;
    }

    return 0;
}

int main(void)
{
    print_ids("Before setuid(getuid())");
    if (try_open_data("before privilege drop") == -1) {
        return EXIT_FAILURE;
    }

    if (setuid(getuid()) == -1) {
        perror("setuid(getuid())");
        return EXIT_FAILURE;
    }

    print_ids("After setuid(getuid())");
    if (try_open_data("after privilege drop") == -1) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
