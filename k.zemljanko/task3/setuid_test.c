#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

void print_uids(void) {
    printf("Real UID: %ld\n", (long)getuid());
    printf("Effective UID: %ld\n", (long)geteuid());
}

void open_file(void) {
    FILE *file = fopen("data.txt", "r");

    if (file == NULL) {
        perror("fopen");
        return;
    }

    printf("data.txt opened successfully\n");
    fclose(file);
}

int main(void) {
    printf("Before setuid:\n");
    print_uids();
    open_file();

    if (setuid(getuid()) == -1) {
        perror("setuid");
        exit(EXIT_FAILURE);
    }

    printf("After setuid:\n");
    print_uids();
    open_file();

    return 0;
}
