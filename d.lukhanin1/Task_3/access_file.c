#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void print_ids(void) {
    printf("Real UID: %ld; effective UID: %ld\n",
           (long)getuid(), (long)geteuid());
    fflush(stdout);
}

static void try_open(const char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        perror(path);
        return;
    }

    printf("Opened %s successfully\n", path);
    if (fclose(file) == EOF) {
        perror("fclose");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s FILE\n", argv[0]);
        return EXIT_FAILURE;
    }

    print_ids();
    try_open(argv[1]);

    if (setuid(getuid()) == -1) {
        perror("setuid");
        return EXIT_FAILURE;
    }

    print_ids();
    try_open(argv[1]);
    return EXIT_SUCCESS;
}
