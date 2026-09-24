#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

extern char **environ;

typedef struct {
    char option;
    char *argument;
} Action;


int parse_number(const char *text, rlim_t *result) {
    char *end;
    long value;

    errno = 0;
    value = strtol(text, &end, 10);

    if (errno != 0 || end == text || *end != '\0' || value < 0) {
        return -1;
    }

    *result = (rlim_t)value;
    return 0;
}


// -i
void print_ids(void) {
    printf("Real UID: %ld\n", (long)getuid());
    printf("Effective UID: %ld\n", (long)geteuid());
    printf("Real GID: %ld\n", (long)getgid());
    printf("Effective GID: %ld\n", (long)getegid());
}


// -s
void become_group_leader(void) {
    if (setpgid(0, 0) == -1) {
        perror("setpgid");
    }
}


// -p
void print_process_ids(void) {
    printf("PID: %ld\n", (long)getpid());
    printf("PPID: %ld\n", (long)getppid());
    printf("PGID: %ld\n", (long)getpgid(0));
}


// -u
void print_ulimit(void) {
    struct rlimit limit;

    if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
        perror("getrlimit");
        return;
    }

    if (limit.rlim_cur == RLIM_INFINITY) {
        printf("Ulimit: unlimited\n");
    } else {
        printf("Ulimit: %llu\n",
               (unsigned long long)limit.rlim_cur);
    }
}


// -U
void change_ulimit(const char *argument) {
    struct rlimit limit;
    rlim_t new_limit;

    if (parse_number(argument, &new_limit) == -1) {
        fprintf(stderr, "Invalid value for -U: %s\n", argument);
        return;
    }

    if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
        perror("getrlimit");
        return;
    }

    if (limit.rlim_max != RLIM_INFINITY &&
        new_limit > limit.rlim_max) {
        fprintf(stderr,
                "Value for -U is greater than maximum limit\n");
        return;
    }

    limit.rlim_cur = new_limit;

    if (setrlimit(RLIMIT_NOFILE, &limit) == -1) {
        perror("setrlimit");
    }
}


// -c
void print_core_limit(void) {
    struct rlimit limit;

    if (getrlimit(RLIMIT_CORE, &limit) == -1) {
        perror("getrlimit");
        return;
    }

    if (limit.rlim_cur == RLIM_INFINITY) {
        printf("Core file size: unlimited\n");
    } else {
        printf("Core file size: %llu bytes\n",
               (unsigned long long)limit.rlim_cur);
    }
}


// -C
void change_core_limit(const char *argument) {
    struct rlimit limit;
    rlim_t new_limit;

    if (parse_number(argument, &new_limit) == -1) {
        fprintf(stderr, "Invalid value for -C: %s\n", argument);
        return;
    }

    if (getrlimit(RLIMIT_CORE, &limit) == -1) {
        perror("getrlimit");
        return;
    }

    if (limit.rlim_max != RLIM_INFINITY &&
        new_limit > limit.rlim_max) {
        fprintf(stderr,
                "Value for -C is greater than maximum limit\n");
        return;
    }

    limit.rlim_cur = new_limit;

    if (setrlimit(RLIMIT_CORE, &limit) == -1) {
        perror("setrlimit");
    }
}


// -d
void print_current_directory(void) {
    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        return;
    }

    printf("Current directory: %s\n", cwd);
}


// -v
void print_environment(void) {
    char **env = environ;

    while (*env != NULL) {
        printf("%s\n", *env);
        env++;
    }
}


// -V
void change_environment(char *argument) {
    if (strchr(argument, '=') == NULL) {
        fprintf(stderr,
                "Invalid value for -V. Expected name=value\n");
        return;
    }

    if (putenv(argument) != 0) {
        perror("putenv");
    }
}


int main(int argc, char *argv[]) {
    Action *actions = NULL;
    int action_count = 0;

    int c;

    opterr = 0;

    while ((c = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {

        if (c == '?') {
            fprintf(stderr, "Invalid option: -%c\n", optopt);
            free(actions);
            return 1;
        }

        Action *new_actions =
            realloc(actions,
                    (action_count + 1) * sizeof(Action));

        if (new_actions == NULL) {
            perror("realloc");
            free(actions);
            return 1;
        }

        actions = new_actions;

        actions[action_count].option = (char)c;

        if (c == 'U' || c == 'C' || c == 'V') {
            actions[action_count].argument = optarg;
        } else {
            actions[action_count].argument = NULL;
        }

        action_count++;
    }


    for (int i = action_count - 1; i >= 0; i--) {

        switch (actions[i].option) {

            case 'i':
                print_ids();
                break;

            case 's':
                become_group_leader();
                break;

            case 'p':
                print_process_ids();
                break;

            case 'u':
                print_ulimit();
                break;

            case 'U':
                change_ulimit(actions[i].argument);
                break;

            case 'c':
                print_core_limit();
                break;

            case 'C':
                change_core_limit(actions[i].argument);
                break;

            case 'd':
                print_current_directory();
                break;

            case 'v':
                print_environment();
                break;

            case 'V':
                change_environment(actions[i].argument);
                break;
        }
    }

    free(actions);

    return 0;
}
