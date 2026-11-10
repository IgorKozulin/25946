#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#ifndef SIGALRM
#define SIGALRM 14
#endif
static unsigned int alarm(unsigned int seconds)
{
    (void)seconds;
    return 0;
}
#endif

#define BUFFER_SIZE 4096

typedef struct {
    off_t offset;
    size_t length;
} LineInfo;

static int g_fd = -1;
static LineInfo *g_table = NULL;
static size_t g_count = 0;

static void add_line(LineInfo **table, size_t *count, size_t *capacity,
                     off_t offset, size_t length)
{
    LineInfo *new_table;

    if (*count == *capacity) {
        *capacity = (*capacity == 0) ? 8 : (*capacity * 2);
        new_table = realloc(*table, *capacity * sizeof(*new_table));
        if (new_table == NULL) {
            perror("realloc");
            free(*table);
            exit(EXIT_FAILURE);
        }
        *table = new_table;
    }

    (*table)[*count].offset = offset;
    (*table)[*count].length = length;
    (*count)++;
}

static size_t build_line_table(int fd, LineInfo **table)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    off_t file_offset = 0;
    off_t line_start = 0;
    size_t current_length = 0;
    size_t count = 0;
    size_t capacity = 0;

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < bytes_read; ++i) {
            if (buffer[i] == '\n') {
                add_line(table, &count, &capacity, line_start, current_length);
                line_start = file_offset + (off_t)i + 1;
                current_length = 0;
            } else {
                ++current_length;
            }
        }
        file_offset += bytes_read;
    }

    if (bytes_read == -1) {
        perror("read");
        free(*table);
        return 0;
    }

    if (current_length > 0) {
        add_line(table, &count, &capacity, line_start, current_length);
    }

    return count;
}

static void print_table(const LineInfo *table, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        printf("Line %zu: Offset = %lld, Length = %zu\n",
               i + 1,
               (long long)table[i].offset,
               table[i].length);
    }
}

static void print_file_contents(int fd)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
        perror("lseek");
        _exit(EXIT_FAILURE);
    }

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        if (write(STDOUT_FILENO, buffer, (size_t)bytes_read) == -1) {
            perror("write");
            _exit(EXIT_FAILURE);
        }
    }

    if (bytes_read == -1) {
        perror("read");
        _exit(EXIT_FAILURE);
    }
}

static void alarm_handler(int sig)
{
    (void)sig;

    write(STDOUT_FILENO, "\n[TIMEOUT] Time is up! Printing full file content...\n",
          strlen("\n[TIMEOUT] Time is up! Printing full file content...\n"));
    print_file_contents(g_fd);
    _exit(EXIT_SUCCESS);
}

static void print_line_by_index(int fd, const LineInfo *table, size_t index)
{
    char *line_buffer = malloc(table[index].length + 1);
    ssize_t bytes_read;

    if (line_buffer == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    if (lseek(fd, table[index].offset, SEEK_SET) == (off_t)-1) {
        perror("lseek");
        free(line_buffer);
        exit(EXIT_FAILURE);
    }

    bytes_read = read(fd, line_buffer, table[index].length);
    if (bytes_read < 0) {
        perror("read");
        free(line_buffer);
        exit(EXIT_FAILURE);
    }

    line_buffer[bytes_read] = '\0';
    printf("%s\n", line_buffer);
    free(line_buffer);
}

int main(int argc, char *argv[])
{
    int chosen_line;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    g_fd = open(argv[1], O_RDONLY);
    if (g_fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    signal(SIGALRM, alarm_handler);

    g_table = NULL;
    g_count = build_line_table(g_fd, &g_table);
    if (g_count == 0) {
        printf("File is empty.\n");
        close(g_fd);
        free(g_table);
        return EXIT_SUCCESS;
    }

    print_table(g_table, g_count);
    printf("-------------------------\n");

    while (1) {
        printf("Enter line number (0 to quit, 5 sec timeout): ");
        fflush(stdout);
        alarm(5);

        if (scanf("%d", &chosen_line) != 1) {
            alarm(0);
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {
            }
            fprintf(stderr, "Invalid input.\n");
            continue;
        }

        alarm(0);

        if (chosen_line == 0) {
            break;
        }

        if (chosen_line < 1 || (size_t)chosen_line > g_count) {
            fprintf(stderr, "Line number is out of range.\n");
            continue;
        }

        print_line_by_index(g_fd, g_table, (size_t)chosen_line - 1);
    }

    close(g_fd);
    free(g_table);
    return EXIT_SUCCESS;
}
