#define _XOPEN_SOURCE 700
#define _FILE_OFFSET_BITS 64

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

struct line_info {
    off_t offset;
    size_t length;
};

struct line_table {
    struct line_info *lines;
    size_t count;
    size_t capacity;
};

static int add_line(struct line_table *table, off_t offset, size_t length)
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

static int build_table(int fd, struct line_table *table)
{
    off_t start = lseek(fd, 0, SEEK_CUR);
    size_t length = 0;
    char character;
    ssize_t received;

    if (start == (off_t)-1) {
        perror("lseek");
        return -1;
    }
    for (;;) {
        received = read(fd, &character, 1);
        if (received == -1 && errno == EINTR) {
            continue;
        }
        if (received == -1) {
            perror("read");
            return -1;
        }
        if (received == 0) {
            break;
        }
        if (length == SIZE_MAX) {
            fputs("Line is too long\n", stderr);
            return -1;
        }
        ++length;
        if (character == '\n') {
            if (add_line(table, start, length) == -1) {
                return -1;
            }
            start = lseek(fd, 0, SEEK_CUR);
            if (start == (off_t)-1) {
                perror("lseek");
                return -1;
            }
            length = 0;
        }
    }
    if (length != 0 && add_line(table, start, length) == -1) {
        return -1;
    }
    return 0;
}

static int read_exact(int fd, char *buffer, size_t length)
{
    size_t done = 0;

    while (done < length) {
        ssize_t received = read(fd, buffer + done, length - done);

        if (received == -1 && errno == EINTR) {
            continue;
        }
        if (received == -1) {
            perror("read");
            return -1;
        }
        if (received == 0) {
            fputs("File changed while reading\n", stderr);
            return -1;
        }
        done += (size_t)received;
    }
    return 0;
}

static int print_line(int fd, const struct line_info *line)
{
    char *buffer;

    if (line->length == SIZE_MAX) {
        fputs("Line is too long\n", stderr);
        return -1;
    }
    buffer = malloc(line->length + 1);
    if (buffer == NULL) {
        perror("malloc");
        return -1;
    }
    if (lseek(fd, line->offset, SEEK_SET) == (off_t)-1) {
        perror("lseek");
        free(buffer);
        return -1;
    }
    if (read_exact(fd, buffer, line->length) == -1) {
        free(buffer);
        return -1;
    }
    buffer[line->length] = '\0';
    printf("%s", buffer);
    if (line->length == 0 || buffer[line->length - 1] != '\n') {
        putchar('\n');
    }
    free(buffer);
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
    char input[128];
    int fd;
    int result = EXIT_SUCCESS;
    size_t index;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return EXIT_FAILURE;
    }
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }
    if (build_table(fd, &table) == -1) {
        result = EXIT_FAILURE;
        goto done;
    }

    puts("--- Debug: Line Table ---");
    for (index = 0; index < table.count; ++index) {
        printf("Line %zu: Offset = %jd, Length = %zu\n", index + 1,
               (intmax_t)table.lines[index].offset, table.lines[index].length);
    }
    puts("-------------------------");

    for (;;) {
        uintmax_t number;

        fputs("Enter line number (0 to quit): ", stdout);
        fflush(stdout);
        if (fgets(input, sizeof(input), stdin) == NULL) {
            if (ferror(stdin)) {
                perror("fgets");
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
        if (print_line(fd, &table.lines[(size_t)number - 1]) == -1) {
            result = EXIT_FAILURE;
            break;
        }
    }

done:
    free(table.lines);
    if (close(fd) == -1) {
        perror("close");
        result = EXIT_FAILURE;
    }
    return result;
}
