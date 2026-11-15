#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
    int pipefd[2];
    pid_t pid;
    char buffer[256];

    if (pipe(pipefd) == -1) {
        perror("Ошибка создания канала");
        return 1;
    }

    pid = fork();

    if (pid == -1) {
        perror("Ошибка создания процесса");
        close(pipefd[0]);
        close(pipefd[1]);
        return 1;
    }

    if (pid == 0) {
        // Потомок читает из канала
        close(pipefd[1]);

        printf("[ПОТОМОК] Ожидаю данные от родителя...\n");
        fflush(stdout);

        char c;
        int i = 0;

        while (read(pipefd[0], &c, 1) > 0) {
            if (c == '\n') {
                buffer[i] = '\0';
                printf("[ПОТОМОК] Результат: %s\n", buffer);
                fflush(stdout);
                i = 0;
            } else if (i < 255) {
                buffer[i++] = toupper((unsigned char)c);
            }
        }

        if (i > 0) {
            buffer[i] = '\0';
            printf("[ПОТОМОК] Результат: %s\n", buffer);
        }

        close(pipefd[0]);
        exit(EXIT_SUCCESS);
    }

    // Родитель записывает в канал
    close(pipefd[0]);

    printf("[РОДИТЕЛЬ] Связь через канал установлена!\n");
    printf("[РОДИТЕЛЬ] Введите текст (пустая строка или Ctrl+D — выход):\n");

    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
            break;

        if (strcmp(buffer, "\n") == 0)
            break;

        if (write(pipefd[1], buffer, strlen(buffer)) == -1) {
            perror("Ошибка записи в канал");
            break;
        }
    }

    close(pipefd[1]);
    wait(NULL);

    printf("[РОДИТЕЛЬ] Потомок завершился. Программа завершена.\n");

    return 0;
}
