#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>

/* Глобальный счетчик сигналов SIGINT. */
volatile sig_atomic_t beep_count = 0;

/* Флаг: включён ли режим реакции на SIGINT (пищалка активна).
 * Переключается по Ctrl+Z (SIGTSTP). */
volatile sig_atomic_t beep_enabled = 1;

/* Обработчик SIGINT: издаем звуковой сигнал и увеличиваем счетчик.
 * Если режим выключен — ничего не делаем. */
static void sigint_handler(int sig)
{
    (void)sig;

    if (!beep_enabled) {
        /* Режим подачи сигнала отключён — игнорируем Ctrl+C. */
        return;
    }

    write(STDOUT_FILENO, "\a", 1);
    beep_count++;
}

/* Обработчик SIGTSTP (Ctrl+Z): переключаем режим вместо приостановки. */
static void sigtstp_handler(int sig)
{
    (void)sig;

    beep_enabled = !beep_enabled;

    if (beep_enabled) {
        const char msg[] = "\n[SIGTSTP] Режим пищалки ВКЛЮЧЁН (Ctrl+C снова пищит)\n";
        write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    } else {
        const char msg[] = "\n[SIGTSTP] Режим пищалки ВЫКЛЮЧЕН (Ctrl+C игнорируется)\n";
        write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    }
}

/* Обработчик SIGQUIT: выводим итоговое количество сигналов и завершаемся. */
static void sigquit_handler(int sig)
{
    (void)sig;

    char buffer[256];
    int len = snprintf(buffer, sizeof(buffer),
                       "\nВсего прозвучало сигналов: %d\n"
                       "Режим пищалки на момент выхода: %s\n"
                       "Программа завершена по SIGQUIT\n",
                       (int)beep_count,
                       beep_enabled ? "ВКЛЮЧЁН" : "ВЫКЛЮЧЕН");

    if (len > 0) {
        write(STDOUT_FILENO, buffer, (size_t)len);
    }

    _exit(EXIT_SUCCESS);
}

/* Отключаем эхо управляющих символов (ECHOCTL), чтобы при нажатии
 * Ctrl+C / Ctrl+Z на экране не печаталось "^C" / "^Z". */
static void disable_echoctl(void)
{
    struct termios tty;

    if (tcgetattr(STDIN_FILENO, &tty) != 0)
        return;

    tty.c_lflag &= ~(tcflag_t)ECHOCTL;
    (void)tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

int main(void)
{
    struct sigaction sa_int;
    struct sigaction sa_quit;
    struct sigaction sa_tstp;

    /* --- SIGINT (Ctrl+C) --- */
    memset(&sa_int, 0, sizeof(sa_int));
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction SIGINT");
        return EXIT_FAILURE;
    }

    /* --- SIGQUIT (Ctrl+\) --- */
    memset(&sa_quit, 0, sizeof(sa_quit));
    sa_quit.sa_handler = sigquit_handler;
    sigemptyset(&sa_quit.sa_mask);
    sa_quit.sa_flags = 0;

    if (sigaction(SIGQUIT, &sa_quit, NULL) == -1) {
        perror("sigaction SIGQUIT");
        return EXIT_FAILURE;
    }

    /* --- SIGTSTP (Ctrl+Z) ---
     * Перехватываем, чтобы НЕ приостанавливать процесс, а переключать
     * режим пищалки. SA_RESTART — чтобы pause() после обработки
     * продолжил ждать сигналов. */
    memset(&sa_tstp, 0, sizeof(sa_tstp));
    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;

    if (sigaction(SIGTSTP, &sa_tstp, NULL) == -1) {
        perror("sigaction SIGTSTP");
        return EXIT_FAILURE;
    }

    /* Убираем "^C"/"^Z" с экрана */
    disable_echoctl();

    printf("Пищалка запущена!\n");
    printf("Ctrl+C  — звуковой сигнал (если режим включён)\n");
    printf("Ctrl+Z  — переключить режим подачи сигнала (вкл/выкл)\n");
    printf("Ctrl+\\  — выход с подсчётом сигналов\n");
    printf("Ожидаю сигналов...\n");
    fflush(stdout);

    while (1) {
        pause();
    }

    return EXIT_SUCCESS;
}
