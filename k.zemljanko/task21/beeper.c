#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

volatile sig_atomic_t beep_count = 0;

void sigint_handler(int sig) {
    write(STDOUT_FILENO, "\a", 1);
    beep_count++;
}

void sigquit_handler(int sig) {
    char num[20];
    int i = 0;
    int count = beep_count;

    const char msg[] = "\nВсего прозвучало сигналов: ";
    const char end[] = "\nПрограмма завершена по SIGQUIT\n";

    // Преобразуем количество сигналов в символы
    do {
        num[i++] = '0' + count % 10;
        count /= 10;
    } while (count > 0);

    write(STDOUT_FILENO, msg, sizeof(msg) - 1);

    while (i > 0) {
        i--;
        write(STDOUT_FILENO, &num[i], 1);
    }

    write(STDOUT_FILENO, end, sizeof(end) - 1);

    _exit(EXIT_SUCCESS);
}

int main(void) {
    struct sigaction sa_int, sa_quit;

    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("Ошибка установки SIGINT");
        return 1;
    }

    sa_quit.sa_handler = sigquit_handler;
    sigemptyset(&sa_quit.sa_mask);
    sa_quit.sa_flags = 0;

    if (sigaction(SIGQUIT, &sa_quit, NULL) == -1) {
        perror("Ошибка установки SIGQUIT");
        return 1;
    }

    printf("Пищалка запущена!\n");
    printf("Нажмите Ctrl+C для звукового сигнала\n");
    printf("Нажмите Ctrl+\\ для завершения программы\n");
    printf("Ожидание сигналов...\n");
    fflush(stdout);

    while (1) {
        pause();
    }

    return 0;
}
