#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t timed_out = 0;
static sigset_t wait_mask;

static void alarm_handler(int signal_number) {
    (void)signal_number;
    timed_out = 1;
}

static int setup_timer(void) {
    struct sigaction action;
    memset(&action, 0, sizeof action);
    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGALRM, &action, NULL) == -1) {
        perror("sigaction");
        return -1;
    }
    sigset_t blocked;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGALRM);
    if (sigprocmask(SIG_BLOCK, &blocked, &wait_mask) == -1) {
        perror("sigprocmask");
        return -1;
    }
    sigdelset(&wait_mask, SIGALRM);
    /* No stdio read-ahead: pselect must see every unread input byte. */
    if (setvbuf(stdin, NULL, _IONBF, 0) != 0) {
        fprintf(stderr, "Cannot disable stdin buffering\n");
        return -1;
    }
    return 0;
}

struct Mapping {
    const char *data;
    size_t size;
};

static int map_file(int fd, struct Mapping *mapping) {
    struct stat info;
    if (fstat(fd, &info) == -1) {
        perror("fstat");
        return -1;
    }
    if (!S_ISREG(info.st_mode) || info.st_size < 0 || (uintmax_t)info.st_size > SIZE_MAX) {
        fprintf(stderr, "Expected a regular file that fits in memory\n");
        return -1;
    }
    mapping->size = (size_t)info.st_size;
    if (mapping->size == 0) {
        return 0;
    }
    void *data = mmap(NULL, mapping->size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        return -1;
    }
    mapping->data = data;
    return 0;
}

struct Line {
    off_t offset;
    size_t length;
};

struct Table {
    struct Line *lines;
    size_t count;
    size_t capacity;
};

static int add_line(struct Table *table, off_t offset, size_t length) {
    if (table->count == table->capacity) {
        size_t capacity = table->capacity == 0 ? 16 : table->capacity * 2;
        if (capacity < table->capacity || capacity > SIZE_MAX / sizeof *table->lines) {
            fprintf(stderr, "Too many lines\n");
            return -1;
        }
        struct Line *lines = realloc(table->lines, capacity * sizeof *lines);
        if (lines == NULL) {
            perror("realloc");
            return -1;
        }
        table->lines = lines;
        table->capacity = capacity;
    }
    table->lines[table->count++] = (struct Line){offset, length};
    return 0;
}

static int build_table(const struct Mapping *mapping, struct Table *table) {
    size_t start = 0;
    for (size_t i = 0; i < mapping->size; ++i) {
        if (mapping->data[i] == '\n') {
            if (add_line(table, (off_t)start, i - start + 1) == -1) {
                return -1;
            }
            start = i + 1;
        }
    }
    return start == mapping->size ? 0 :
        add_line(table, (off_t)start, mapping->size - start);
}

static int print_line(const struct Mapping *mapping, const struct Line *line) {
    const char *text = mapping->data + (size_t)line->offset;
    /* mmap data has no terminating NUL; output precisely length bytes. */
    if (fwrite(text, 1, line->length, stdout) != line->length) {
        perror("fwrite");
        return -1;
    }
    if (text[line->length - 1] != '\n' && putchar('\n') == EOF) {
        return -1;
    }
    return 0;
}

/* Consume a whole input line, even when the number is too long. */
static int read_query(char *buffer, size_t size) {
    size_t length = 0;
    int too_long = 0;
    int status = 0;
    timed_out = 0;
    alarm(5);
    for (;;) {
        fd_set input;
        FD_ZERO(&input);
        FD_SET(STDIN_FILENO, &input);
        /* SIGALRM is unblocked atomically only while waiting here. */
        int ready = pselect(STDIN_FILENO + 1, &input, NULL, NULL, NULL, &wait_mask);
        if (timed_out) {
            status = 3;
            break;
        }
        if (ready == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("pselect");
            status = -1;
            break;
        }
        int ch = getchar();
        if (ch == EOF || ch == '\n') {
            if (ferror(stdin)) {
                perror("getchar");
                status = -1;
            } else if (ch == EOF && length == 0 && !too_long) {
                status = 0;
            } else {
                status = too_long ? 2 : 1;
            }
            break;
        }
        if (length + 1 < size) {
            buffer[length++] = (char)ch;
        } else {
            too_long = 1;
        }
    }
    alarm(0);
    /* Also catch expiry between input readiness and timer cancellation. */
    sigset_t pending;
    if (sigpending(&pending) == -1) {
        perror("sigpending");
        status = -1;
    } else if (sigismember(&pending, SIGALRM) == 1) {
        status = 3;
    }
    buffer[length] = '\0';
    return status;
}

static int print_file(const struct Mapping *mapping) {
    if (mapping->size != 0 &&
        fwrite(mapping->data, 1, mapping->size, stdout) != mapping->size) {
        perror("fwrite");
        return -1;
    }
    return 0;
}

static int parse_number(char *text, size_t *number) {
    while (isspace((unsigned char)*text)) {
        ++text;
    }
    if (!isdigit((unsigned char)*text)) {
        return -1;
    }
    errno = 0;
    char *end;
    uintmax_t value = strtoumax(text, &end, 10);
    if (errno == ERANGE || value > SIZE_MAX) {
        return -1;
    }
    while (isspace((unsigned char)*end)) {
        ++end;
    }
    if (*end != '\0') {
        return -1;
    }
    *number = (size_t)value;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3 || (argc == 3 && strcmp(argv[2], "--table") != 0)) {
        fprintf(stderr, "Usage: %s FILE [--table]\n", argv[0]);
        return EXIT_FAILURE;
    }
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }
    struct Mapping mapping = {NULL, 0};
    struct Table table = {NULL, 0, 0};
    int result = EXIT_FAILURE;
    if (map_file(fd, &mapping) == -1) {
        goto cleanup;
    }
    if (close(fd) == -1) {
        perror("close");
        fd = -1;
        goto cleanup;
    }
    fd = -1; /* Closing fd does not remove the mapping. */
    if (build_table(&mapping, &table) == -1) {
        goto cleanup;
    }
    if (argc == 3) {
        for (size_t i = 0; i < table.count; ++i) {
            printf("Line %zu: offset=%jd length=%zu\n", i + 1,
                   (intmax_t)table.lines[i].offset, table.lines[i].length);
        }
    }
    if (setup_timer() == -1) {
        goto cleanup;
    }
    result = EXIT_SUCCESS;
    for (;;) {
        char buffer[128];
        size_t number;
        fflush(stdout);
        fprintf(stderr, "Line number (0 to quit; 5 seconds): ");
        int status = read_query(buffer, sizeof buffer);
        if (status == 3) {
            fprintf(stderr, "\nTimeout: printing the whole file\n");
            if (print_file(&mapping) == -1) {
                result = EXIT_FAILURE;
            }
            break;
        }
        if (status <= 0) {
            if (status == -1) {
                result = EXIT_FAILURE;
            }
            break;
        }
        if (status == 2 || parse_number(buffer, &number) == -1) {
            fprintf(stderr, "Enter a nonnegative integer\n");
            continue;
        }
        if (number == 0) {
            break;
        }
        if (number > table.count) {
            fprintf(stderr, "No such line; file contains %zu lines\n", table.count);
            continue;
        }
        if (print_line(&mapping, &table.lines[number - 1]) == -1) {
            result = EXIT_FAILURE;
            break;
        }
    }
cleanup:
    free(table.lines);
    if (mapping.data != NULL && munmap((void *)mapping.data, mapping.size) == -1) {
        perror("munmap");
        result = EXIT_FAILURE;
    }
    if (fd != -1 && close(fd) == -1) {
        perror("close");
        result = EXIT_FAILURE;
    }
    if (fflush(stdout) == EOF) {
        perror("fflush");
        result = EXIT_FAILURE;
    }
    return result;
}
