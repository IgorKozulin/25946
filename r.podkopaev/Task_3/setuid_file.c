#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>

static void check_file(void)
{
    printf("Real UID: %lu, Effective UID: %lu\n",
           (unsigned long)getuid(), (unsigned long)geteuid());
    FILE *file = fopen("data.txt", "r");
    if (file == NULL) {
        perror("fopen");
    } else {
        puts("File opened successfully");
        if (fclose(file) == EOF) perror("fclose");
    }
}

int main(void)
{
    puts("Before setuid(getuid()):");
    check_file();
    if (setuid(getuid()) == -1) { perror("setuid"); return 1; }
    puts("After setuid(getuid()):");
    check_file();
    return 0;
}
