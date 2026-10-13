#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

typedef struct {
    long offset;
    int length;
} LineInfo;

static int g_fd = -1;
static LineInfo *g_table = NULL;
static int g_num_lines = 0;

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

void alarm_handler(int sig)
{
    (void)sig;
    const char msg[] = "\n[TIMEOUT] Time is up! Printing full file content...\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);

    if (lseek(g_fd, 0, SEEK_SET) == -1) {
        _exit(EXIT_FAILURE);
    }

    char buf[1024];
    ssize_t n;
    while ((n = read(g_fd, buf, sizeof(buf))) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t w = write(STDOUT_FILENO, buf + written, n - written);
            if (w == -1 && errno == EINTR) continue;
            if (w <= 0) {
                _exit(EXIT_FAILURE);
            }
            written += w;
        }
    }

    _exit(n == -1 ? EXIT_FAILURE : EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    g_fd = open(argv[1], O_RDONLY);
    if (g_fd == -1) {
        perror("open");
        return 1;
    }

    int capacity = 0;
    char ch;
    long line_start = 0;
    int line_length = 0;

    ssize_t scan_result;
    while ((scan_result = read(g_fd, &ch, 1)) == 1) {
        if (ch == '\n') {
            if (g_num_lines == capacity) {
                capacity = capacity == 0 ? 16 : capacity * 2;
                LineInfo *tmp = realloc(g_table, capacity * sizeof(LineInfo));
                if (tmp == NULL) {
                    fprintf(stderr, "Error: realloc failed\n");
                    close(g_fd);
                    free(g_table);
                    return 1;
                }
                g_table = tmp;
            }
            g_table[g_num_lines].offset = line_start;
            g_table[g_num_lines].length = line_length;
            g_num_lines++;
            line_start = lseek(g_fd, 0, SEEK_CUR);
            if (line_start == -1) {
                perror("lseek");
                free(g_table);
                close(g_fd);
                return 1;
            }
            line_length = 0;
        } else {
            line_length++;
        }
    }

    if (scan_result == -1) {
        perror("read");
        free(g_table);
        close(g_fd);
        return 1;
    }

    if (line_length > 0) {
        if (g_num_lines == capacity) {
            capacity = capacity == 0 ? 16 : capacity * 2;
            LineInfo *tmp = realloc(g_table, capacity * sizeof(LineInfo));
            if (tmp == NULL) {
                fprintf(stderr, "Error: realloc failed\n");
                close(g_fd);
                free(g_table);
                return 1;
            }
            g_table = tmp;
        }
        g_table[g_num_lines].offset = line_start;
        g_table[g_num_lines].length = line_length;
        g_num_lines++;
    }

    printf("--- Debug: Line Table ---\n");
    for (int i = 0; i < g_num_lines; i++) {
        printf("Line %d: Offset = %ld, Length = %d\n",
               i + 1, g_table[i].offset, g_table[i].length);
    }
    printf("-------------------------\n");

    if (signal(SIGALRM, alarm_handler) == SIG_ERR) {
        perror("signal");
        free(g_table);
        close(g_fd);
        return 1;
    }

    int line_num;
    while (1) {
        printf("Enter line number (0 to quit, 5 sec timeout): ");
        fflush(stdout);

        alarm(5);

        int result = scanf("%d", &line_num);

        alarm(0);

        if (result != 1) {
            break;
        }

        if (line_num == 0) {
            break;
        }

        if (line_num < 1 || line_num > g_num_lines) {
            printf("Invalid line number. Valid range: 1..%d\n", g_num_lines);
            continue;
        }

        long offset = g_table[line_num - 1].offset;
        int length = g_table[line_num - 1].length;

        if (lseek(g_fd, offset, SEEK_SET) == -1) {
            perror("lseek");
            continue;
        }

        char *buffer = malloc(length + 1);
        if (buffer == NULL) {
            fprintf(stderr, "Error: malloc failed\n");
            continue;
        }

        ssize_t bytes_read = read_full(g_fd, buffer, (size_t)length);
        if (bytes_read == -1) {
            perror("read");
            free(buffer);
            continue;
        }

        buffer[bytes_read] = '\0';
        printf("%s\n", buffer);
        free(buffer);
    }

    free(g_table);
    close(g_fd);
    return 0;
}
