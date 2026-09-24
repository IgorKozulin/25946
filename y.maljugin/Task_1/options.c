#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <errno.h>
#include <string.h>

extern char **environ;

int main(int argc, char *argv[])
{
    const char *options = "ispuU:cC:dvV:";
    size_t capacity = 1;
    for (int j = 1; j < argc; j++)
        capacity += strlen(argv[j]);
    int *opts = malloc(capacity * sizeof(*opts));
    char **args = malloc(capacity * sizeof(*args));
    if (opts == NULL || args == NULL) {
        perror("malloc");
        free(opts);
        free(args);
        return 1;
    }
    int count = 0;
    int c;
    int status = 0;

    opterr = 0;

    while ((c = getopt(argc, argv, options)) != -1) {
        opts[count] = c;
        args[count] = optarg;
        count++;
    }

    for (int i = count - 1; i >= 0; i--) {
        struct rlimit lim;
        char cwd[1024];

        switch (opts[i]) {
        case 'i':
            printf("UID: %ld EUID: %ld\n", (long)getuid(), (long)geteuid());
            printf("GID: %ld EGID: %ld\n", (long)getgid(), (long)getegid());
            break;

        case 's':
            if (setpgid(0, 0) == -1)
                perror("setpgid");
            break;

        case 'p':
            printf("PID: %ld\n", (long)getpid());
            printf("PPID: %ld\n", (long)getppid());
            printf("PGID: %ld\n", (long)getpgrp());
            break;

        case 'u':
            if (getrlimit(RLIMIT_NOFILE, &lim) == -1)
                perror("getrlimit");
            else
                printf("ulimit: %llu\n",
                       (unsigned long long)lim.rlim_cur);
            break;

        case 'U': {
            char *end;
            errno = 0;
            long value = strtol(args[i], &end, 10);

            if (errno == ERANGE || end == args[i] || *end != '\0' || value < 0) {
                fprintf(stderr, "Invalid value for -U: %s\n", args[i]);
                status = 1;
                break;
            }

            if (getrlimit(RLIMIT_NOFILE, &lim) == -1) {
                perror("getrlimit");
                break;
            }

            lim.rlim_cur = value;

            if (setrlimit(RLIMIT_NOFILE, &lim) == -1) {
                perror("setrlimit");
                status = 1;
            }

            break;
        }

        case 'c':
            if (getrlimit(RLIMIT_CORE, &lim) == -1)
                perror("getrlimit");
            else
                printf("Core size: %llu\n",
                       (unsigned long long)lim.rlim_cur);
            break;

        case 'C': {
            char *end;
            errno = 0;
            long value = strtol(args[i], &end, 10);

            if (errno == ERANGE || end == args[i] || *end != '\0' || value < 0) {
                fprintf(stderr, "Invalid value for -C: %s\n", args[i]);
                status = 1;
                break;
            }

            if (getrlimit(RLIMIT_CORE, &lim) == -1) {
                perror("getrlimit");
                break;
            }

            lim.rlim_cur = value;

            if (setrlimit(RLIMIT_CORE, &lim) == -1) {
                perror("setrlimit");
                status = 1;
            }

            break;
        }

        case 'd':
            if (getcwd(cwd, sizeof(cwd)) != NULL)
                printf("%s\n", cwd);
            else
                perror("getcwd");
            break;

        case 'v':
            for (char **e = environ; *e != NULL; e++)
                printf("%s\n", *e);
            break;

        case 'V':
            if (strchr(args[i], '=') == NULL || args[i][0] == '=') {
                fprintf(stderr, "Invalid value for -V: %s\n", args[i]);
                status = 1;
            } else if (putenv(args[i]) != 0) {
                perror("putenv");
                status = 1;
            }
            break;

        case '?':
            fprintf(stderr, "Invalid option: -%c\n", optopt);
            status = 1;
            break;
        }
    }

    free(opts);
    free(args);
    return status;
}
