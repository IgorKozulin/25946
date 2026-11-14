#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFER_SIZE 4096 // размер буфера для чтения файла

// информация об одной строке: где начинается и сколько символов
typedef struct {
    off_t offset;    // смещение от начала файла
    size_t length;   // длина строки без '\n'
} LineInfo;

// добавить новую строку в динамическую таблицу
static void add_line(LineInfo **table, size_t *count, size_t *capacity,
                     off_t offset, size_t length)
{
    LineInfo *new_table;

    // если места нет, увеличиваю массив
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

    // сохраняю данные о строке
    (*table)[*count].offset = offset;
    (*table)[*count].length = length;
    (*count)++;
}

// читаю файл и строю таблицу всех строк
static size_t build_line_table(int fd, LineInfo **table)
{
    char buffer[BUFFER_SIZE]; // буфер для чтения
    ssize_t bytes_read;       // сколько прочитано
    off_t file_offset = 0;    // текущая позиция в файле
    off_t line_start = 0;     // начало текущей строки
    size_t current_length = 0; // длина текущей строки
    size_t count = 0;         // количество строк
    size_t capacity = 0;      // вместимость таблицы

    // читаю файл по частям
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        // прохожу по всем символам в буфере
        for (ssize_t i = 0; i < bytes_read; ++i) {
            if (buffer[i] == '\n') {
                // нашли конец строки — записываю её
                add_line(table, &count, &capacity, line_start, current_length);
                line_start = file_offset + (off_t)i + 1;
                current_length = 0;
            } else {
                // обычный символ — увеличиваю длину
                ++current_length;
            }
        }
        file_offset += bytes_read;
    }

    // если read вернул ошибку
    if (bytes_read == -1) {
        perror("read");
        free(*table);
        return 0;
    }

    // если файл не заканчивается '\n', добавляю последнюю строку
    if (current_length > 0) {
        add_line(table, &count, &capacity, line_start, current_length);
    }

    return count;
}

// печатаю таблицу строк: номер, смещение, длину и вес строки в файле
static void print_table(const LineInfo *table, size_t count, off_t file_size)
{
    off_t total = 0;

    for (size_t i = 0; i < count; ++i) {
        // строка кончается там, где начинается следующая, то есть вместе с '\n'
        off_t bytes = (i + 1 < count) ? (table[i + 1].offset - table[i].offset)
                                      : (file_size - table[i].offset);

        printf("Line %zu: Offset = %lld, Length = %zu, Size = %lld\n",
               i + 1,
               (long long)table[i].offset,
               table[i].length,
               (long long)bytes);

        total += bytes;
    }

    printf("File size = %lld bytes\n", (long long)total);
}

// читаю и печатаю строку по её индексу
static void print_line_by_index(int fd, const LineInfo *table, size_t index)
{
    char *line_buffer;
    ssize_t bytes_read;

    // выделяю память под строку, +1 для '\0'
    line_buffer = malloc(table[index].length + 1);
    if (line_buffer == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // перехожу к нужному месту в файле
    if (lseek(fd, table[index].offset, SEEK_SET) == (off_t)-1) {
        perror("lseek");
        free(line_buffer);
        exit(EXIT_FAILURE);
    }

    // читаю ровно длину строки
    bytes_read = read(fd, line_buffer, table[index].length);
    if (bytes_read < 0) {
        perror("read");
        free(line_buffer);
        exit(EXIT_FAILURE);
    }

    line_buffer[bytes_read] = '\0'; // завершаю строку
    printf("%s\n", line_buffer);
    free(line_buffer);
}

int main(int argc, char *argv[])
{
    int fd;                 // дескриптор файла
    LineInfo *table = NULL; // таблица строк
    size_t count = 0;       // количество строк
    int chosen_line;        // выбранный номер строки
    struct stat st;         // чтобы узнать размер файла

    // проверяю аргументы командной строки
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // открываю файл только для чтения
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    // узнаю размер файла
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return EXIT_FAILURE;
    }

    // строю таблицу строк
    count = build_line_table(fd, &table);
    if (count == 0) {
        printf("File is empty.\n");
        close(fd);
        free(table);
        return EXIT_SUCCESS;
    }

    // показываю таблицу строк вместе с размером каждой строки и всего файла
    print_table(table, count, st.st_size);

    // цикл запроса номера строки
    while (1) {
        printf("Enter line number (0 to quit): ");
        if (scanf("%d", &chosen_line) != 1) {
            fprintf(stderr, "Invalid input.\n");
            return EXIT_FAILURE;
        }

        // 0 — выход
        if (chosen_line == 0) {
            break;
        }

        // проверка диапазона
        if (chosen_line < 1 || (size_t)chosen_line > count) {
            fprintf(stderr, "Line number is out of range.\n");
            continue;
        }

        // печатаю выбранную строку
        print_line_by_index(fd, table, (size_t)chosen_line - 1);
    }

    close(fd);
    free(table);
    return EXIT_SUCCESS;
}