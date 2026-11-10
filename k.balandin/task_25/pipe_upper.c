#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <stdint.h>

#define BUF_SIZE 1024

int main(void)
{
    int pipefd[2];      /* родитель → потомок: текст */
    int stat_pipe[2];   /* потомок → родитель: статистика */
    pid_t pid;

    /* 1. Создаём оба канала */
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }
    if (pipe(stat_pipe) == -1) {
        perror("pipe stat");
        close(pipefd[0]);
        close(pipefd[1]);
        return EXIT_FAILURE;
    }

    /* 2. Создаём подпроцесс */
    pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    /* ================= ДОЧЕРНИЙ ПРОЦЕСС ================= */
    if (pid == 0) {
        /* Закрываем ненужные концы:
         * - pipefd[1]  — потомок не пишет в текстовый канал
         * - stat_pipe[0] — потомок не читает из канала статистики */
        close(pipefd[1]);
        close(stat_pipe[0]);

        printf("[CHILD] Запущен. Ожидаю данные от родителя...\n");
        fflush(stdout);

        char buffer[BUF_SIZE];
        ssize_t n;
        unsigned long total_chars = 0;   /* общее число прочитанных символов */

        /* Читаем из текстового канала до EOF */
        while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            total_chars += (unsigned long)n;

            for (ssize_t i = 0; i < n; i++) {
                buffer[i] = (char)toupper((unsigned char)buffer[i]);
            }

            fwrite(buffer, 1, (size_t)n, stdout);
            fflush(stdout);
        }

        if (n == -1) {
            perror("read");
            close(pipefd[0]);
            close(stat_pipe[1]);
            _exit(EXIT_FAILURE);
        }

        close(pipefd[0]);

        /* Отправляем родителю количество символов.
         * Используем фиксированный тип uint64_t — чтобы обе стороны
         * точно понимали размер. */
        uint64_t stat = (uint64_t)total_chars;
        ssize_t w = write(stat_pipe[1], &stat, sizeof(stat));
        if (w == -1) {
            perror("write stat");
            close(stat_pipe[1]);
            _exit(EXIT_FAILURE);
        }
        close(stat_pipe[1]);   /* закрываем — родитель получит EOF, если что */

        printf("\n[CHILD] Конец данных (EOF). Обработано символов: %lu\n",
               total_chars);
        fflush(stdout);
        exit(EXIT_SUCCESS);
    }

    /* ================= РОДИТЕЛЬСКИЙ ПРОЦЕСС ================= */
    /* Закрываем ненужные концы:
     * - pipefd[0]    — родитель не читает из текстового канала
     * - stat_pipe[1] — родитель не пишет в канал статистики */
    close(pipefd[0]);
    close(stat_pipe[1]);

    printf("[PARENT] Связь через канал установлена!\n");
    printf("[PARENT] Введите текст (пустая строка или Ctrl+D — выход):\n");
    fflush(stdout);

    char line[BUF_SIZE];

    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n[PARENT] EOF — завершаю ввод.\n");
            break;
        }

        if (line[0] == '\n') {
            printf("[PARENT] Пустая строка — завершаю ввод.\n");
            break;
        }

        size_t len = strlen(line);
        size_t written = 0;
        while (written < len) {
            ssize_t w = write(pipefd[1], line + written, len - written);
            if (w == -1) {
                perror("write");
                close(pipefd[1]);
                close(stat_pipe[0]);
                wait(NULL);
                return EXIT_FAILURE;
            }
            written += (size_t)w;
        }
    }

    /* Сигнал EOF потомку: закрываем конец записи */
    close(pipefd[1]);

    /* Читаем статистику от потомка.
     * read может вернуть меньше sizeof(uint64_t), поэтому читаем в цикле. */
    uint64_t stat = 0;
    size_t got = 0;
    while (got < sizeof(stat)) {
        ssize_t r = read(stat_pipe[0], (char *)&stat + got, sizeof(stat) - got);
        if (r == -1) {
            perror("read stat");
            close(stat_pipe[0]);
            wait(NULL);
            return EXIT_FAILURE;
        }
        if (r == 0) break;   /* EOF раньше времени */
        got += (size_t)r;
    }
    close(stat_pipe[0]);

    /* Дожидаемся завершения потомка */
    wait(NULL);

    /* Выводим статистику */
    printf("\n");
    printf("========== СТАТИСТИКА ==========\n");
    if (got == sizeof(stat)) {
        printf("Всего обработано символов: %llu\n",
               (unsigned long long)stat);
    } else {
        printf("Не удалось получить статистику от потомка (получено %zu байт)\n",
               got);
    }
    printf("===============================\n");
    printf("[PARENT] Потомок завершился. Родитель тоже завершает работу.\n");

    return EXIT_SUCCESS;
}
