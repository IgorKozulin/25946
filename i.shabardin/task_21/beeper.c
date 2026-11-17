#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static volatile sig_atomic_t beep_count = 0;
static volatile sig_atomic_t finished = 0;

/* В обработчиках нет printf: write безопасен при получении сигнала. */
static void on_int(int signal_number)
{
    (void)signal_number;
    if (beep_count < 1000000) {
        ++beep_count;
    }
    (void)write(STDOUT_FILENO, "\a", 1);
}

static void on_quit(int signal_number)
{
    (void)signal_number;
    finished = 1;
}

int main(void)
{
    struct sigaction action;
    sigset_t blocked, previous, empty;

    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = on_int;
    if (sigaction(SIGINT, &action, NULL) == -1) {
        perror("sigaction SIGINT");
        return EXIT_FAILURE;
    }
    action.sa_handler = on_quit;
    if (sigaction(SIGQUIT, &action, NULL) == -1) {
        perror("sigaction SIGQUIT");
        return EXIT_FAILURE;
    }

    /* Блокируем SIGQUIT между проверкой флага и сном: иначе можно зависнуть. */
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGQUIT);
    if (sigprocmask(SIG_BLOCK, &blocked, &previous) == -1) {
        perror("sigprocmask");
        return EXIT_FAILURE;
    }
    sigemptyset(&empty);
    puts("Ctrl+C: beep; Ctrl+\\: finish and print count.");
    fflush(stdout);
    while (!finished) {
        if (sigsuspend(&empty) == -1 && errno != EINTR) {
            perror("sigsuspend");
            return EXIT_FAILURE;
        }
    }
    if (sigprocmask(SIG_SETMASK, &previous, NULL) == -1) {
        perror("sigprocmask restore");
        return EXIT_FAILURE;
    }
    printf("Total beeps: %d\n", (int)beep_count);
    return EXIT_SUCCESS;
}
