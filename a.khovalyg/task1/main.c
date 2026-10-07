#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

extern char **environ;

int main(int argc, char *argv[]) {
    int c;

    while ((c = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {
        switch (c) {
            case 'i': {
                printf("Real UID=%d, Effective UID=%d\n", getuid(), geteuid());
                printf("Real GID=%d, Effective GID=%d\n", getgid(), getegid());
                break;
            }
            case 's': {
                if (setpgid(0, 0) == -1) {
                    perror("setpgid failed");
                } else {
                    printf("Process became group leader (PGID=%d)\n", getpid());
                }
                break;
            }
            case 'p': {
                printf("PID=%d, PPID=%d, PGID=%d\n", getpid(), getppid(), getpgrp());
                break;
            }
            case 'u': {
                struct rlimit rl;
                if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
                    printf("Current ulimit (RLIMIT_NOFILE): %lu\n",
                           (unsigned long)rl.rlim_cur);
                } else {
                    perror("getrlimit failed");
                }
                break;
            }
            case 'U': {
                long val = strtol(optarg, NULL, 10);
                if (val < 0) {
                    fprintf(stderr, "Invalid ulimit value: %s\n", optarg);
                    return 1;
                }
                struct rlimit rl;
                if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
                    rl.rlim_cur = val;
                    if (setrlimit(RLIMIT_NOFILE, &rl) == -1) {
                        perror("setrlimit failed");
                    } else {
                        printf("Ulimit set to %ld\n", val);
                    }
                }
                break;
            }
            case 'c': {
                struct rlimit rl;
                if (getrlimit(RLIMIT_CORE, &rl) == 0) {
                    printf("Core file size limit: %lu bytes\n",
                           (unsigned long)rl.rlim_cur);
                } else {
                    perror("getrlimit CORE failed");
                }
                break;
            }
            case 'C': {
                long val = strtol(optarg, NULL, 10);
                if (val < 0) {
                    fprintf(stderr, "Invalid core size: %s\n", optarg);
                    return 1;
                }
                struct rlimit rl;
                if (getrlimit(RLIMIT_CORE, &rl) == 0) {
                    rl.rlim_cur = val;
                    if (setrlimit(RLIMIT_CORE, &rl) == -1) {
                        perror("setrlimit CORE failed");
                    } else {
                        printf("Core size set to %ld bytes\n", val);
                    }
                }
                break;
            }
            case 'd': {
                char buf[PATH_MAX];
                if (getcwd(buf, sizeof(buf)) != NULL) {
                    printf("Current directory: %s\n", buf);
                } else {
                    perror("getcwd failed");
                }
                break;
            }
            case 'v': {
                for (char **env = environ; *env != NULL; env++) {
                    printf("%s\n", *env);
                }
                break;
            }
            case 'V': {
                if (putenv(optarg) != 0) {
                    perror("putenv failed");
                } else {
                    printf("Environment variable set: %s\n", optarg);
                }
                break;
            }
            case '?':
                fprintf(stderr, "Unknown option: -%c\n", optopt);
                return 1;
            default:
                abort();
        }
    }

    return 0;
}
