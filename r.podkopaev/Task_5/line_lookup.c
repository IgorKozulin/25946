#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

typedef struct {
    long offset;
    int length;
} LineInfo;

static ssize_t read_full(int fd, char *buffer, size_t length)
{
    size_t total = 0;
    while (total < length) {
        ssize_t n = read(fd, buffer + total, length - total);
        if (n == -1 && errno == EINTR) continue;
        if (n <= 0) {
            if (n == 0) errno = EIO;
            return -1;
        }
        total += (size_t)n;
    }
    return (ssize_t)total;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    LineInfo *table = NULL;
    int num_lines = 0;
    int capacity = 0;

    char ch;
    long line_start = 0;
    int line_length = 0;

    ssize_t scan_result;
    while ((scan_result = read(fd, &ch, 1)) == 1) {
        if (ch == '\n') {
            if (num_lines == capacity) {
                capacity = capacity == 0 ? 16 : capacity * 2;
                LineInfo *tmp = realloc(table, capacity * sizeof(LineInfo));
                if (tmp == NULL) {
                    fprintf(stderr, "Error: realloc failed\n");
                    close(fd);
                    free(table);
                    return 1;
                }
                table = tmp;
            }
            table[num_lines].offset = line_start;
            table[num_lines].length = line_length;
            num_lines++;
            line_start = lseek(fd, 0, SEEK_CUR);
            if (line_start == -1) {
                perror("lseek");
                free(table);
                close(fd);
                return 1;
            }
            line_length = 0;
        } else {
            line_length++;
        }
    }

    if (scan_result == -1) {
        perror("read");
        free(table);
        close(fd);
        return 1;
    }

    if (line_length > 0) {
        if (num_lines == capacity) {
            capacity = capacity == 0 ? 16 : capacity * 2;
            LineInfo *tmp = realloc(table, capacity * sizeof(LineInfo));
            if (tmp == NULL) {
                fprintf(stderr, "Error: realloc failed\n");
                close(fd);
                free(table);
                return 1;
            }
            table = tmp;
        }
        table[num_lines].offset = line_start;
        table[num_lines].length = line_length;
        num_lines++;
    }

    printf("--- Debug: Line Table ---\n");
    for (int i = 0; i < num_lines; i++) {
        printf("Line %d: Offset = %ld, Length = %d\n",
               i + 1, table[i].offset, table[i].length);
    }
    printf("-------------------------\n");

    int line_num;
    while (1) {
        printf("Enter line number (0 to quit): ");
        fflush(stdout);

        if (scanf("%d", &line_num) != 1) {
            break;
        }

        if (line_num == 0) {
            break;
        }

        if (line_num < 1 || line_num > num_lines) {
            printf("Invalid line number. Valid range: 1..%d\n", num_lines);
            continue;
        }

        long offset = table[line_num - 1].offset;
        int length = table[line_num - 1].length;

        if (lseek(fd, offset, SEEK_SET) == -1) {
            perror("lseek");
            continue;
        }

        char *buffer = malloc(length + 1);
        if (buffer == NULL) {
            fprintf(stderr, "Error: malloc failed\n");
            continue;
        }

        ssize_t bytes_read = read_full(fd, buffer, (size_t)length);
        if (bytes_read == -1) {
            perror("read");
            free(buffer);
            continue;
        }

        buffer[bytes_read] = '\0';
        printf("%s\n", buffer);
        free(buffer);
    }

    free(table);
    close(fd);
    return 0;
}
