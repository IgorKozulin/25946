#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <limits.h>

extern char **environ;

int process_options(int argc, char *argv[]) {
    // All options: s, p, u, d don't need arguments
    char *options = "ispuU:cC:dvV:";
    int c;
    int result;
    int invalid_option;
    char *value;
    char path[PATH_MAX];
    char **variable;
    long new_limit;
    struct rlimit limit;

    // Parse options
    c = getopt(argc, argv, options);
    if (c == -1) {
        return 0;
    }

    value = optarg;
    invalid_option = optopt;
    result = process_options(argc, argv);

    switch (c) {
        case 'i':
            printf("Option -i detected\n");
            printf("Real UID: %d\n", (int)getuid());
            printf("Effective UID: %d\n", (int)geteuid());
            printf("Real GID: %d\n", (int)getgid());
            printf("Effective GID: %d\n", (int)getegid());
            break;
        case 's':
            printf("Option -s detected\n");
            setpgid(0, 0);
            break;
        case 'p':
            printf("Option -p detected\n");
            printf("PID: %d\n", (int)getpid());
            printf("PPID: %d\n", (int)getppid());
            printf("PGID: %d\n", (int)getpgrp());
            break;
        case 'u':
            printf("Option -u detected\n");
            getrlimit(RLIMIT_NOFILE, &limit);
            if (limit.rlim_cur == RLIM_INFINITY) {
                printf("Ulimit: unlimited\n");
            } else {
                printf("Ulimit: %llu\n", (unsigned long long)limit.rlim_cur);
            }
            break;
        case 'U':
            printf("Option -U with value: %s\n", value);
            new_limit = atol(value);
            if (new_limit < 0) {
                printf("Invalid value for -U: %s\n", value);
                result = 1;
            } else {
                getrlimit(RLIMIT_NOFILE, &limit);
                limit.rlim_cur = (rlim_t)new_limit;
                setrlimit(RLIMIT_NOFILE, &limit);
            }
            break;
        case 'c':
            printf("Option -c detected\n");
            getrlimit(RLIMIT_CORE, &limit);
            if (limit.rlim_cur == RLIM_INFINITY) {
                printf("Core size: unlimited\n");
            } else {
                printf("Core size: %llu\n", (unsigned long long)limit.rlim_cur);
            }
            break;
        case 'C':
            printf("Option -C with value: %s\n", value);
            new_limit = atol(value);
            if (new_limit < 0) {
                printf("Invalid value for -C: %s\n", value);
                result = 1;
            } else {
                getrlimit(RLIMIT_CORE, &limit);
                limit.rlim_cur = (rlim_t)new_limit;
                setrlimit(RLIMIT_CORE, &limit);
            }
            break;
        case 'd':
            printf("Option -d detected\n");
            if (getcwd(path, sizeof(path)) != NULL) {
                printf("Directory: %s\n", path);
            }
            break;
        case 'v':
            printf("Option -v detected\n");
            for (variable = environ; *variable != NULL; variable++) {
                printf("%s\n", *variable);
            }
            break;
        case 'V':
            printf("Option -V with value: %s\n", value);
            if (strchr(value, '=') == NULL) {
                printf("Invalid value for -V: %s\n", value);
                result = 1;
            } else {
                putenv(value);
            }
            break;
        case '?':
            printf("Invalid option: %c\n", invalid_option);
            result = 1;
            break;
    }

    return result;
}

int main(int argc, char *argv[]) {
    opterr = 0;
    return process_options(argc, argv);
}
