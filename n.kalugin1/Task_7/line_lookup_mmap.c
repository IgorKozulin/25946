#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// заглушки для Windows, чтобы код собирался
#ifdef _WIN32
#include <windows.h>
#include <io.h>

#ifndef PROT_READ
#define PROT_READ 0x1
#endif
#ifndef MAP_PRIVATE
#define MAP_PRIVATE 0x02
#endif
#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif
#ifndef SIGALRM
#define SIGALRM 14
#endif

// заглушка alarm для Windows
static unsigned int alarm(unsigned int seconds)
{
    (void)seconds;
    return 0;
}

// своя реализация mmap через WinAPI
static void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
    HANDLE file_handle;
    HANDLE mapping_handle;
    void *view;

    (void)addr;
    (void)prot;
    (void)flags;
    (void)offset;

    // получаю HANDLE из дескриптора файла
    file_handle = (HANDLE)_get_osfhandle(fd);
    if (file_handle == INVALID_HANDLE_VALUE) {
        return MAP_FAILED;
    }

    // создаю отображение файла
    mapping_handle = CreateFileMapping(file_handle, NULL, PAGE_READONLY, 0, 0, NULL);
    if (mapping_handle == NULL) {
        return MAP_FAILED;
    }

    // получаю указатель на данные
    view = MapViewOfFile(mapping_handle, FILE_MAP_READ, 0, 0, length);
    CloseHandle(mapping_handle);
    if (view == NULL) {
        return MAP_FAILED;
    }

    return view;
}

// своя реализация munmap через WinAPI
static int munmap(void *addr, size_t length)
{
    (void)length;
    return UnmapViewOfFile(addr) ? 0 : -1;
}
#else
#include <sys/mman.h> // на Unix mmap/munmap уже есть
#endif

#define BUFFER_SIZE 4096

// информация о строке: смещение и длина
typedef struct {
    off_t offset;    // где начинается строка
    size_t length;   // длина без '\n'
} LineInfo;

// глобальные переменные, чтобы обработчик сигнала имел доступ
static char *g_map = NULL;       // указатель на отображённый файл
static size_t g_map_size = 0;    // размер отображения
static LineInfo *g_table = NULL; // таблица строк
static size_t g_count = 0;       // количество строк

// добавить строку в динамическую таблицу
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

// строю таблицу строк прямо по отображённой памяти
static void build_line_table(const char *data, size_t size)
{
    off_t line_start = 0;      // начало текущей строки
    size_t current_length = 0; // длина текущей строки
    size_t capacity = 0;       // вместимость таблицы

    // прохожу по всем байтам файла
    for (size_t i = 0; i < size; ++i) {
        if (data[i] == '\n') {
            // конец строки — записываю её
            add_line(&g_table, &g_count, &capacity, line_start, current_length);
            line_start = (off_t)i + 1;
            current_length = 0;
        } else {
            // обычный символ
            ++current_length;
        }
    }

    // если последняя строка без '\n', добавляю её
    if (current_length > 0) {
        add_line(&g_table, &g_count, &capacity, line_start, current_length);
    }
}

// печатаю таблицу строк
static void print_table(const LineInfo *table, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        printf("Line %zu: Offset = %lld, Length = %zu\n",
               i + 1,
               (long long)table[i].offset,
               table[i].length);
    }
}

// вывожу весь файл (используется в обработчике сигнала)
static void print_full_file(void)
{
    if (g_map != NULL && g_map_size > 0) {
        if (write(STDOUT_FILENO, g_map, g_map_size) == -1) {
            perror("write");
            _exit(EXIT_FAILURE);
        }
    }
}

// обработчик SIGALRM: срабатывает по таймауту
static void alarm_handler(int sig)
{
    (void)sig;

    // сообщаю о таймауте
    write(STDOUT_FILENO, "\n[TIMEOUT] Time is up! Printing full file content...\n",
          strlen("\n[TIMEOUT] Time is up! Printing full file content...\n"));
    // печатаю весь файл и выхожу
    print_full_file();
    _exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    int fd;
    struct stat st;
    int chosen_line;

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

    if (st.st_size == 0) {
        printf("File is empty.\n");
        close(fd);
        return EXIT_SUCCESS;
    }

    // отображаю файл в память — читать потом можно без read/lseek
    g_map = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (g_map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return EXIT_FAILURE;
    }
    g_map_size = (size_t)st.st_size;
    close(fd); // дескриптор больше не нужен, отображение живёт само

    build_line_table(g_map, g_map_size);
    if (g_count == 0) {
        printf("File is empty.\n");
        munmap(g_map, g_map_size);
        return EXIT_SUCCESS;
    }

    // показываю таблицу строк
    print_table(g_table, g_count);
    printf("-------------------------\n");

    // устанавливаю обработчик сигнала SIGALRM
    signal(SIGALRM, alarm_handler);

    // цикл запроса номера строки
    while (1) {
        printf("Enter line number (0 to quit, 5 sec timeout): ");
        fflush(stdout);
        alarm(5); // запускаю таймер на 5 секунд

        if (scanf("%d", &chosen_line) != 1) {
            alarm(0); // отменяю таймер
            int ch;
            // очищаю буфер ввода
            while ((ch = getchar()) != '\n' && ch != EOF) {
            }
            fprintf(stderr, "Invalid input.\n");
            continue;
        }

        alarm(0); // ввод получен — отменяю таймер

        // 0 — выход
        if (chosen_line == 0) {
            break;
        }

        // проверка диапазона
        if (chosen_line < 1 || (size_t)chosen_line > g_count) {
            fprintf(stderr, "Line number is out of range.\n");
            continue;
        }

        // печатаю строку прямо из отображённой памяти
        if (write(STDOUT_FILENO, g_map + g_table[chosen_line - 1].offset,
                  g_table[chosen_line - 1].length) == -1) {
            perror("write");
            munmap(g_map, g_map_size);
            free(g_table);
            return EXIT_FAILURE;
        }
        putchar('\n');
    }

    free(g_table);
    munmap(g_map, g_map_size);
    return EXIT_SUCCESS;
}