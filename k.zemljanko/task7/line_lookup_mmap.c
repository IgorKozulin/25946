#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>

typedef struct {
    long offset;
    int length;
} LineInfo;

char *global_map = NULL;
size_t global_size = 0;

void alarm_handler(int sig) {
    const char message[] =
        "\n[TIMEOUT] Time is up! Printing full file content...\n";

    write(STDOUT_FILENO, message, sizeof(message) - 1);
    write(STDOUT_FILENO, global_map, global_size);

    _exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    int fd;
    struct stat st;

    LineInfo *lines = NULL;
    int num_lines = 0;
    int capacity = 0;

    long line_start = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);

    if (fd == -1) {
        perror("open");
        return 1;
    }

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

    global_size = st.st_size;

    global_map = mmap(NULL,
                      global_size,
                      PROT_READ,
                      MAP_PRIVATE,
                      fd,
                      0);

    if (global_map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    close(fd);

    for (size_t i = 0; i < global_size; i++) {
        if (global_map[i] == '\n') {
            if (num_lines == capacity) {
                capacity = capacity == 0 ? 10 : capacity * 2;

                LineInfo *new_lines =
                    realloc(lines, capacity * sizeof(LineInfo));

                if (new_lines == NULL) {
                    perror("realloc");
                    free(lines);
                    munmap(global_map, global_size);
                    return 1;
                }

                lines = new_lines;
            }

            lines[num_lines].offset = line_start;
            lines[num_lines].length =
                (int)(i - line_start + 1);

            num_lines++;
            line_start = i + 1;
        }
    }

    if ((size_t)line_start < global_size) {
        if (num_lines == capacity) {
            capacity = capacity == 0 ? 10 : capacity * 2;

            LineInfo *new_lines =
                realloc(lines, capacity * sizeof(LineInfo));

            if (new_lines == NULL) {
                perror("realloc");
                free(lines);
                munmap(global_map, global_size);
                return 1;
            }

            lines = new_lines;
        }

        lines[num_lines].offset = line_start;
        lines[num_lines].length =
            (int)(global_size - line_start);

        num_lines++;
    }

    printf("--- Debug: Line Table ---\n");

    for (int i = 0; i < num_lines; i++) {
        printf("Line %d: Offset = %ld, Length = %d\n",
               i + 1,
               lines[i].offset,
               lines[i].length);
    }

    printf("-------------------------\n");

    signal(SIGALRM, alarm_handler);

    while (1) {
        int line_number;

        printf("Enter line number (0 to quit, 5 sec timeout): ");
        fflush(stdout);

        alarm(5);

        if (scanf("%d", &line_number) != 1) {
            int c;

            alarm(0);

            while ((c = getchar()) != '\n' && c != EOF) {
            }

            continue;
        }

        alarm(0);

        if (line_number == 0) {
            break;
        }

        if (line_number < 1 || line_number > num_lines) {
            printf("Invalid line number\n");
            continue;
        }

        LineInfo info = lines[line_number - 1];

        fwrite(global_map + info.offset,
               1,
               info.length,
               stdout);

        if (info.length > 0 &&
            global_map[info.offset + info.length - 1] != '\n') {
            printf("\n");
        }
    }

    alarm(0);
    free(lines);
    munmap(global_map, global_size);

    return 0;
}
