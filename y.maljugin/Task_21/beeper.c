#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static volatile sig_atomic_t beeps;
static volatile sig_atomic_t finished;

static void on_interrupt(int signal_number)
{
    (void)signal_number;
    ++beeps;
    (void)write(STDOUT_FILENO, "\a", 1);
}

static void on_quit(int signal_number)
{
    (void)signal_number;
    finished = 1;
}

int main(void)
{
    sigset_t blocked, previous, waiting;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGINT);
    sigaddset(&blocked, SIGQUIT);
    if (sigprocmask(SIG_BLOCK, &blocked, &previous) == -1) {
        perror("sigprocmask");
        return EXIT_FAILURE;
    }

    struct sigaction action = {0};
    sigemptyset(&action.sa_mask);
    action.sa_handler = on_interrupt;
    if (sigaction(SIGINT, &action, NULL) == -1) {
        perror("sigaction SIGINT");
        return EXIT_FAILURE;
    }
    action.sa_handler = on_quit;
    if (sigaction(SIGQUIT, &action, NULL) == -1) {
        perror("sigaction SIGQUIT");
        return EXIT_FAILURE;
    }

    waiting = previous;
    sigdelset(&waiting, SIGINT);
    sigdelset(&waiting, SIGQUIT);
    puts("Press Ctrl-C to beep; Ctrl-\\ to stop.");
    fflush(stdout);
    while (!finished)
        sigsuspend(&waiting);

    if (sigprocmask(SIG_SETMASK, &previous, NULL) == -1) {
        perror("sigprocmask restore");
        return EXIT_FAILURE;
    }
    printf("\nBeeps: %d\n", (int)beeps);
    return EXIT_SUCCESS;
}
