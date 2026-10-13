#define _XOPEN_SOURCE 700
#define _FILE_OFFSET_BITS 64

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

struct line_info {
    size_t offset;
    size_t length;
};

struct line_table {
    struct line_info *lines;
    size_t count;
    size_t capacity;
};

static volatile sig_atomic_t timed_out = 0;

static void alarm_handler(int signal_number)
{
    (void)signal_number;
    timed_out = 1;
}

static int add_line(struct line_table *table, size_t offset, size_t length)
{
    if (table->count == table->capacity) {
        size_t next = table->capacity == 0 ? 16 : table->capacity * 2;
        struct line_info *larger;

        if (next <= table->capacity ||
            next > SIZE_MAX / sizeof(*table->lines)) {
            fputs("Too many lines\n", stderr);
            return -1;
        }
        larger = realloc(table->lines, next * sizeof(*table->lines));
        if (larger == NULL) {
            perror("realloc");
            return -1;
        }
        table->lines = larger;
        table->capacity = next;
    }
    table->lines[table->count].offset = offset;
    table->lines[table->count].length = length;
    ++table->count;
    return 0;
}

static int build_table(const char *map, size_t size, struct line_table *table)
{
    size_t start = 0;
    size_t position;

    for (position = 0; position < size; ++position) {
        if (map[position] == '\n') {
            if (add_line(table, start, position - start + 1) == -1) {
                return -1;
            }
            start = position + 1;
        }
    }
    if (start < size && add_line(table, start, size - start) == -1) {
        return -1;
    }
    return 0;
}

static int print_bytes(const char *data, size_t length)
{
    if (length != 0 && fwrite(data, 1, length, stdout) != length) {
        perror("fwrite");
        return -1;
    }
    if (length != 0 && data[length - 1] != '\n') {
        putchar('\n');
    }
    return 0;
}

static int parse_number(const char *text, uintmax_t *number)
{
    char *end;

    while (isspace((unsigned char)*text)) {
        ++text;
    }
    if (*text == '-' || *text == '\0') {
        return -1;
    }
    errno = 0;
    *number = strtoumax(text, &end, 10);
    if (errno == ERANGE || end == text) {
        return -1;
    }
    while (isspace((unsigned char)*end)) {
        ++end;
    }
    return *end == '\0' ? 0 : -1;
}

int main(int argc, char **argv)
{
    struct line_table table = {NULL, 0, 0};
    struct sigaction action;
    struct stat info;
    char input[128];
    char *map;
    size_t size;
    size_t index;
    int fd;
    int result = EXIT_SUCCESS;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return EXIT_FAILURE;
    }
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }
    if (fstat(fd, &info) == -1) {
        perror("fstat");
        close(fd);
        return EXIT_FAILURE;
    }
    if (info.st_size < 0 || (uintmax_t)info.st_size > SIZE_MAX) {
        fputs("File is too large\n", stderr);
        close(fd);
        return EXIT_FAILURE;
    }
    size = (size_t)info.st_size;
    if (size == 0) {
        puts("File is empty.");
        close(fd);
        return EXIT_SUCCESS;
    }

    map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return EXIT_FAILURE;
    }
    if (close(fd) == -1) {
        perror("close");
        munmap(map, size);
        return EXIT_FAILURE;
    }
    if (build_table(map, size, &table) == -1) {
        result = EXIT_FAILURE;
        goto done;
    }

    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGALRM, &action, NULL) == -1) {
        perror("sigaction");
        result = EXIT_FAILURE;
        goto done;
    }

    puts("--- Debug: Line Table ---");
    for (index = 0; index < table.count; ++index) {
        printf("Line %zu: Offset = %zu, Length = %zu\n", index + 1,
               table.lines[index].offset, table.lines[index].length);
    }
    puts("-------------------------");

    for (;;) {
        uintmax_t number;

        fputs("Enter line number (0 to quit, 5 sec timeout): ", stdout);
        fflush(stdout);
        timed_out = 0;
        alarm(5);
        if (fgets(input, sizeof(input), stdin) == NULL) {
            alarm(0);
            if (timed_out) {
                clearerr(stdin);
                puts("\n[TIMEOUT] Time is up! Printing full file content...");
                if (print_bytes(map, size) == -1) {
                    result = EXIT_FAILURE;
                }
            } else if (ferror(stdin)) {
                perror("fgets");
                result = EXIT_FAILURE;
            }
            break;
        }
        alarm(0);
        if (timed_out) {
            puts("\n[TIMEOUT] Time is up! Printing full file content...");
            if (print_bytes(map, size) == -1) {
                result = EXIT_FAILURE;
            }
            break;
        }
        if (parse_number(input, &number) == -1) {
            fputs("Please enter a nonnegative line number.\n", stderr);
            continue;
        }
        if (number == 0) {
            break;
        }
        if (number > table.count) {
            fputs("Line number is out of range.\n", stderr);
            continue;
        }
        if (print_bytes(map + table.lines[(size_t)number - 1].offset,
                        table.lines[(size_t)number - 1].length) == -1) {
            result = EXIT_FAILURE;
            break;
        }
    }

done:
    free(table.lines);
    if (munmap(map, size) == -1) {
        perror("munmap");
        result = EXIT_FAILURE;
    }
    return result;
}
