#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

typedef struct {
    long offset;
    int length;
} LineInfo;

static char *g_map = NULL;
static size_t g_size = 0;
static LineInfo *g_table = NULL;
static int g_num_lines = 0;

void alarm_handler(int sig)
{
    (void)sig;
    const char msg[] = "\n[TIMEOUT] Time is up! Printing full file content...\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);

    size_t written = 0;
    while (written < g_size) {
        ssize_t w = write(STDOUT_FILENO, g_map + written, g_size - written);
        if (w == -1 && errno == EINTR) continue;
        if (w <= 0) {
            _exit(EXIT_FAILURE);
        }
        written += (size_t)w;
    }

    _exit(EXIT_SUCCESS);
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

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return 1;
    }

    if (st.st_size == 0) {
        printf("File is empty.\n");
        close(fd);
        return 0;
    }

    g_size = (size_t)st.st_size;

    g_map = mmap(NULL, g_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (g_map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    close(fd);

    int capacity = 0;
    long line_start = 0;
    int line_length = 0;

    for (size_t i = 0; i < g_size; i++) {
        if (g_map[i] == '\n') {
            if (g_num_lines == capacity) {
                capacity = capacity == 0 ? 16 : capacity * 2;
                LineInfo *tmp = realloc(g_table, capacity * sizeof(LineInfo));
                if (tmp == NULL) {
                    fprintf(stderr, "Error: realloc failed\n");
                    munmap(g_map, g_size);
                    free(g_table);
                    return 1;
                }
                g_table = tmp;
            }
            g_table[g_num_lines].offset = line_start;
            g_table[g_num_lines].length = line_length;
            g_num_lines++;
            line_start = (long)i + 1;
            line_length = 0;
        } else {
            line_length++;
        }
    }

    if (line_length > 0) {
        if (g_num_lines == capacity) {
            capacity = capacity == 0 ? 16 : capacity * 2;
            LineInfo *tmp = realloc(g_table, capacity * sizeof(LineInfo));
            if (tmp == NULL) {
                fprintf(stderr, "Error: realloc failed\n");
                munmap(g_map, g_size);
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
        munmap(g_map, g_size);
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

        fwrite(g_map + offset, 1, (size_t)length, stdout);
        printf("\n");
    }

    free(g_table);
    munmap(g_map, g_size);
    return 0;
}
