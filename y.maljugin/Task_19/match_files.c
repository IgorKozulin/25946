#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pattern_match.h"

int main(void)
{
    char pattern[4096];
    printf("Pattern: ");
    fflush(stdout);
    if (fgets(pattern, sizeof(pattern), stdin) == NULL)
        return EXIT_FAILURE;
    if (strchr(pattern, '\n') == NULL && !feof(stdin)) {
        fprintf(stderr, "Pattern is too long.\n");
        return EXIT_FAILURE;
    }
    pattern[strcspn(pattern, "\r\n")] = '\0';
    if (strchr(pattern, '/') != NULL) {
        fprintf(stderr, "Task 19 patterns cannot contain '/'.\n");
        return EXIT_FAILURE;
    }

    DIR *directory = opendir(".");
    if (directory == NULL) {
        perror("opendir");
        return EXIT_FAILURE;
    }
    int found = 0;
    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        if (matches(pattern, entry->d_name)) {
            puts(entry->d_name);
            found = 1;
        }
    }
    if (!found)
        puts(pattern);
    closedir(directory);
    return EXIT_SUCCESS;
}
