#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <signal.h>
#include <sys/select.h>
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

static int build_table(int fd, struct Table *table) {
    char buffer[4096];
    off_t start = 0;
    size_t length = 0;
    for (;;) {
        ssize_t count = read(fd, buffer, sizeof buffer);
        if (count == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("read");
            return -1;
        }
        if (count == 0) {
            break;
        }
        /* SEEK_CUR is 1: the position just after this block. */
        off_t end = lseek(fd, 0L, SEEK_CUR);
        if (end == (off_t)-1) {
            perror("lseek");
            return -1;
        }
        off_t block_start = end - count;
        for (ssize_t i = 0; i < count; ++i) {
            if (length == SIZE_MAX - 1) {
                fprintf(stderr, "Line is too long\n");
                return -1;
            }
            ++length;
            if (buffer[i] == '\n') {
                if (add_line(table, start, length) == -1) {
                    return -1;
                }
                start = block_start + i + 1;
                length = 0;
            }
        }
    }
    return length == 0 ? 0 : add_line(table, start, length);
}

/* A read may return fewer bytes than requested. */
static int print_line(int fd, const struct Line *line) {
    char *text = malloc(line->length + 1);
    if (text == NULL) {
        perror("malloc");
        return -1;
    }
    if (lseek(fd, line->offset, SEEK_SET) == (off_t)-1) {
        perror("lseek");
        free(text);
        return -1;
    }
    size_t done = 0;
    while (done < line->length) {
        size_t chunk = line->length - done;
        if (chunk > 4096) {
            chunk = 4096;
        }
        ssize_t count = read(fd, text + done, chunk);
        if (count == -1 && errno == EINTR) {
            continue;
        }
        if (count <= 0) {
            if (count == -1) {
                perror("read");
            } else {
                fprintf(stderr, "File changed: unexpected EOF\n");
            }
            free(text);
            return -1;
        }
        done += (size_t)count;
    }
    text[done] = '\0';
    int result = printf("%s", text) < 0 ? -1 : 0;
    if (done != 0 && text[done - 1] != '\n' && putchar('\n') == EOF) {
        result = -1;
    }
    free(text);
    return result;
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

static int print_file(int fd) {
    if (lseek(fd, 0L, SEEK_SET) == (off_t)-1) {
        perror("lseek");
        return -1;
    }
    char buffer[4097];
    for (;;) {
        ssize_t count = read(fd, buffer, sizeof buffer - 1);
        if (count == -1 && errno == EINTR) {
            continue;
        }
        if (count == -1) {
            perror("read");
            return -1;
        }
        if (count == 0) {
            return 0;
        }
        buffer[count] = '\0';
        if (printf("%s", buffer) < 0) {
            return -1;
        }
    }
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
    struct Table table = {NULL, 0, 0};
    int result = EXIT_FAILURE;
    if (build_table(fd, &table) == -1) {
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
            if (print_file(fd) == -1) {
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
        if (print_line(fd, &table.lines[number - 1]) == -1) {
            result = EXIT_FAILURE;
            break;
        }
    }
cleanup:
    free(table.lines);
    if (close(fd) == -1) {
        perror("close");
        result = EXIT_FAILURE;
    }
    if (fflush(stdout) == EOF) {
        perror("fflush");
        result = EXIT_FAILURE;
    }
    return result;
}
