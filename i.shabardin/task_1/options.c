#define _XOPEN_SOURCE 700

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

extern char **environ;

struct requested_option {
    int name;
    const char *argument;
};

static int add_option(struct requested_option **options, size_t *count,
                      size_t *capacity, int name, const char *argument)
{
    if (*count == *capacity) {
        size_t new_capacity = *capacity == 0 ? 8 : *capacity * 2;
        struct requested_option *new_options;

        if (new_capacity < *capacity ||
            new_capacity > SIZE_MAX / sizeof(**options)) {
            fputs("Too many options\n", stderr);
            return -1;
        }
        new_options = realloc(*options, new_capacity * sizeof(**options));
        if (new_options == NULL) {
            perror("realloc");
            return -1;
        }
        *options = new_options;
        *capacity = new_capacity;
    }

    (*options)[*count].name = name;
    (*options)[*count].argument = argument;
    ++*count;
    return 0;
}

static int parse_limit(const char *text, int option, rlim_t *value)
{
    char *end;
    uintmax_t number;

    if (text == NULL || *text == '\0' || *text == '-') {
        fprintf(stderr, "Invalid value for -%c: %s\n", option,
                text == NULL ? "(missing)" : text);
        return -1;
    }

    errno = 0;
    number = strtoumax(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0' ||
        (uintmax_t)(rlim_t)number != number) {
        fprintf(stderr, "Invalid value for -%c: %s\n", option, text);
        return -1;
    }

    *value = (rlim_t)number;
    return 0;
}

static int print_limit(int resource, const char *label)
{
    struct rlimit limit;

    if (getrlimit(resource, &limit) == -1) {
        perror("getrlimit");
        return -1;
    }
    if (limit.rlim_cur == RLIM_INFINITY) {
        printf("%s: unlimited\n", label);
    } else {
        printf("%s: %" PRIuMAX "\n", label, (uintmax_t)limit.rlim_cur);
    }
    return 0;
}

static int change_limit(int resource, const char *text, int option)
{
    struct rlimit limit;
    rlim_t new_value;

    if (parse_limit(text, option, &new_value) == -1) {
        return -1;
    }
    if (getrlimit(resource, &limit) == -1) {
        perror("getrlimit");
        return -1;
    }
    limit.rlim_cur = new_value;
    if (setrlimit(resource, &limit) == -1) {
        perror("setrlimit");
        return -1;
    }
    return 0;
}

static int print_directory(void)
{
    size_t size = 256;

    for (;;) {
        char *directory = malloc(size);
        int error;

        if (directory == NULL) {
            perror("malloc");
            return -1;
        }
        if (getcwd(directory, size) != NULL) {
            puts(directory);
            free(directory);
            return 0;
        }

        error = errno;
        free(directory);
        if (error != ERANGE) {
            errno = error;
            perror("getcwd");
            return -1;
        }
        if (size > SIZE_MAX / 2) {
            fputs("Working directory path is too long\n", stderr);
            return -1;
        }
        size *= 2;
    }
}

static int change_environment(const char *argument)
{
    const char *equals = strchr(argument, '=');
    char *assignment;

    if (equals == NULL || equals == argument) {
        fprintf(stderr, "Invalid value for -V: %s\n", argument);
        return -1;
    }
    assignment = malloc(strlen(argument) + 1);
    if (assignment == NULL) {
        perror("malloc");
        return -1;
    }
    strcpy(assignment, argument);
    if (putenv(assignment) != 0) {
        perror("putenv");
        free(assignment);
        return -1;
    }
    /* putenv keeps this string; it must remain allocated. */
    return 0;
}

static int run_option(const struct requested_option *option)
{
    switch (option->name) {
    case 'i':
        printf("uid: %lu, euid: %lu, gid: %lu, egid: %lu\n",
               (unsigned long)getuid(), (unsigned long)geteuid(),
               (unsigned long)getgid(), (unsigned long)getegid());
        return 0;
    case 's':
        if (setpgid(0, 0) == -1) {
            perror("setpgid");
            return -1;
        }
        return 0;
    case 'p':
        printf("pid: %ld, ppid: %ld, pgid: %ld\n",
               (long)getpid(), (long)getppid(), (long)getpgrp());
        return 0;
    case 'u':
        return print_limit(RLIMIT_NOFILE, "ulimit (open files)");
    case 'U':
        return change_limit(RLIMIT_NOFILE, option->argument, 'U');
    case 'c':
        return print_limit(RLIMIT_CORE, "core file size (bytes)");
    case 'C':
        return change_limit(RLIMIT_CORE, option->argument, 'C');
    case 'd':
        return print_directory();
    case 'v': {
        char **entry;
        for (entry = environ; entry != NULL && *entry != NULL; ++entry) {
            puts(*entry);
        }
        return 0;
    }
    case 'V':
        return change_environment(option->argument);
    default:
        return -1;
    }
}

int main(int argc, char **argv)
{
    struct requested_option *options = NULL;
    size_t count = 0;
    size_t capacity = 0;
    int option;
    int result = 0;

    if (argc == 1) {
        printf("Usage: %s [-i] [-s] [-p] [-u] [-Uvalue] [-c] [-Csize] [-d] [-v] [-Vname=value]\n", argv[0]);
        return 0;
    }

    opterr = 0;
    while ((option = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {
        if (option == '?') {
            if (optopt == 'U' || optopt == 'C' || optopt == 'V') {
                fprintf(stderr, "Option -%c requires a value\n", optopt);
            } else {
                fprintf(stderr, "Unknown option: -%c\n", optopt);
            }
            free(options);
            return 2;
        }
        if (add_option(&options, &count, &capacity, option, optarg) == -1) {
            free(options);
            return 1;
        }
    }
    if (optind != argc) {
        fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
        free(options);
        return 2;
    }

    while (count > 0) {
        --count;
        if (run_option(&options[count]) == -1) {
            result = 1;
            break;
        }
    }
    free(options);
    return result;
}
