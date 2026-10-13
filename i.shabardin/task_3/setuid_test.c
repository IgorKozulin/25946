#define _XOPEN_SOURCE 700

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

static void print_ids(const char *stage)
{
    printf("%s: real UID = %lu, effective UID = %lu\n", stage,
           (unsigned long)getuid(), (unsigned long)geteuid());
}

static void check_file(const char *stage, const char *path)
{
    FILE *file = fopen(path, "r");

    if (file == NULL) {
        fprintf(stderr, "%s: cannot open %s: %s\n", stage, path,
                strerror(errno));
        return;
    }
    printf("%s: opened %s successfully\n", stage, path);
    if (fclose(file) == EOF) {
        perror("fclose");
    }
}

int main(int argc, char **argv)
{
    const char *path;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [file]\n", argv[0]);
        return EXIT_FAILURE;
    }
    path = argc == 2 ? argv[1] : "data.txt";

    print_ids("Before setuid");
    check_file("Before setuid", path);

    if (setuid(getuid()) == -1) {
        perror("setuid");
        return EXIT_FAILURE;
    }

    print_ids("After setuid");
    check_file("After setuid", path);
    return EXIT_SUCCESS;
}
