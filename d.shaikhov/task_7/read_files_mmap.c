#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

typedef struct {
    long offset;
    int length;
} LineInfo;

char *file_map;
size_t file_size;

void alarm_handler(int sig) {
    char message[] = "\n[TIMEOUT] Time is up! Printing full file content...\n";

    (void)sig;

    write(STDOUT_FILENO, message, sizeof(message) - 1);
    write(STDOUT_FILENO, file_map, file_size);

    _exit(0);
}

int main(int argc, char *argv[]) {
    int file_fd;
    struct stat st;

    LineInfo *table = NULL;
    int num_lines = 0;
    int capacity = 0;

    long line_start = 0;
    long current_pos;

    file_fd = open(argv[1], O_RDONLY);

    if (file_fd == -1) {
        printf("Cannot open file\n");
        return 1;
    }

    if (fstat(file_fd, &st) == -1) {
        printf("fstat error\n");
        close(file_fd);
        return 1;
    }

    file_size = st.st_size;

    if (file_size == 0) {
        printf("File is empty.\n");
        close(file_fd);
        return 0;
    }

    file_map = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, file_fd, 0);

    if (file_map == MAP_FAILED) {
        printf("mmap error\n");
        close(file_fd);
        return 1;
    }

    close(file_fd);

    for (size_t i = 0; i < file_size; i++) {
        if (file_map[i] == '\n') {
            current_pos = i + 1;

            if (num_lines == capacity) {
                LineInfo *new_table;

                if (capacity == 0) {
                    capacity = 10;
                } else {
                    capacity = capacity * 2;
                }

                new_table = realloc(table, capacity * sizeof(LineInfo));

                if (new_table == NULL) {
                    printf("Memory allocation error\n");
                    free(table);
                    munmap(file_map, file_size);
                    return 1;
                }

                table = new_table;
            }

            table[num_lines].offset = line_start;
            table[num_lines].length = current_pos - line_start;

            num_lines++;

            line_start = current_pos;
        }
    }

    current_pos = file_size;

    if (current_pos > line_start) {
        if (num_lines == capacity) {
            LineInfo *new_table;

            if (capacity == 0) {
                capacity = 10;
            } else {
                capacity = capacity * 2;
            }

            new_table = realloc(table, capacity * sizeof(LineInfo));

            if (new_table == NULL) {
                printf("Memory allocation error\n");
                free(table);
                munmap(file_map, file_size);
                return 1;
            }

            table = new_table;
        }

        table[num_lines].offset = line_start;
        table[num_lines].length = current_pos - line_start;

        num_lines++;
    }

    printf("--- Line Table ---\n");

    for (int i = 0; i < num_lines; i++) {
    	double size_kb = table[i].length / 1024.0;

    	printf("Line %d: Offset = %ld, Length = %d, Size = %.3f KB\n", i + 1, table[i].offset, table[i].length, size_kb);
}

    printf("------------------\n");

    signal(SIGALRM, alarm_handler);

    while (1) {
        int number;
        int index;

        printf("Enter line number (0 to quit, 5 sec timeout): ");
        fflush(stdout);

        alarm(5);

        if (scanf("%d", &number) != 1) {
            alarm(0);

            printf("Invalid input\n");

            while (getchar() != '\n') {
            }

            continue;
        }

        alarm(0);

        if (number == 0) {
            break;
        }

        if (number < 1 || number > num_lines) {
            printf("Invalid line number\n");
            continue;
        }

        index = number - 1;

        fwrite(file_map + table[index].offset, 1, table[index].length, stdout);

        if (table[index].length > 0 && file_map[table[index].offset + table[index].length - 1] != '\n') {
            printf("\n");
        }
    }

    free(table);
    munmap(file_map, file_size);

    return 0;
}
