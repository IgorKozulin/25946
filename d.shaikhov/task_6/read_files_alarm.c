#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

typedef struct {
    long offset;
    int length;
} LineInfo;

int file_fd;

void alarm_handler(int sig) {
    char buffer[1024];
    int bytes_read;

    (void)sig;

    write(STDOUT_FILENO, "\n[TIMEOUT] Time is up! Printing full file content...\n", 53);

    lseek(file_fd, 0, SEEK_SET);

    while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
        write(STDOUT_FILENO, buffer, bytes_read);
    }

    _exit(0);
}

int main(int argc, char *argv[]) {
    char c;

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

    while (read(file_fd, &c, 1) > 0) {
        if (c == '\n') {
            current_pos = lseek(file_fd, 0, SEEK_CUR);

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
                    close(file_fd);
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

    current_pos = lseek(file_fd, 0, SEEK_CUR);

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
                close(file_fd);
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
        printf("Line %d: Offset = %ld, Length = %d\n", i + 1, table[i].offset, table[i].length);
    }

    printf("------------------\n");

    signal(SIGALRM, alarm_handler);

    while (1) {
        int number;
        int index;
        char *buffer;
        int bytes_read;

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

        if (lseek(file_fd, table[index].offset, SEEK_SET) == -1) {

            printf("lseek error\n");
            continue;
        }

        buffer = malloc(table[index].length + 1);

        if (buffer == NULL) {
            printf("Memory allocation error\n");
            break;
        }

        bytes_read = read(file_fd, buffer, table[index].length);

        if (bytes_read == -1) {
            printf("Read error\n");
            free(buffer);
            continue;
        }

        buffer[bytes_read] = '\0';

        printf("%s", buffer);

        if (bytes_read > 0 && buffer[bytes_read - 1] != '\n') {

            printf("\n");
        }

        free(buffer);
    }

    free(table);
    close(file_fd);

    return 0;
}
 
 

