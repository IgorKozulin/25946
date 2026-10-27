#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "pattern_match.h"

static int found;
static int failed;

static char *join_path(const char *directory, const char *name)
{
    size_t first = strlen(directory), second = strlen(name);
    int slash = first != 0 && directory[first - 1] != '/';
    char *path = malloc(first + slash + second + 1);
    if (path == NULL)
        return NULL;
    memcpy(path, directory, first);
    if (slash)
        path[first++] = '/';
    memcpy(path + first, name, second + 1);
    return path;
}

static void expand(const char *directory, const char *remaining)
{
    const char *slash = strchr(remaining, '/');
    size_t length = slash ? (size_t)(slash - remaining) : strlen(remaining);
    char *component = strndup(remaining, length);
    if (component == NULL) {
        failed = 1;
        return;
    }

    if (strpbrk(component, "*?") == NULL) {
        char *path = join_path(directory, component);
        if (path == NULL)
            failed = 1;
        else {
            struct stat info;
            if (lstat(path, &info) == 0) {
                if (slash != NULL) {
                    if (S_ISDIR(info.st_mode))
                        expand(path, slash + 1);
                } else {
                    puts(path);
                    found = 1;
                }
            }
            free(path);
        }
    } else {
        DIR *stream = opendir(*directory ? directory : ".");
        if (stream != NULL) {
            struct dirent *entry;
            while ((entry = readdir(stream)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 ||
                    !matches(component, entry->d_name))
                    continue;
                char *path = join_path(directory, entry->d_name);
                if (path == NULL) {
                    failed = 1;
                    break;
                }
                if (slash != NULL) {
                    struct stat info;
                    if (lstat(path, &info) == 0 && S_ISDIR(info.st_mode))
                        expand(path, slash + 1);
                } else {
                    puts(path);
                    found = 1;
                }
                free(path);
            }
            closedir(stream);
        }
    }
    free(component);
}

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
    if (*pattern == '/')
        expand("/", pattern + 1);
    else
        expand("", pattern);
    if (failed)
        perror("pattern expansion");
    else if (!found)
        puts(pattern);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
