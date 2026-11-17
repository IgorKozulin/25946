#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>

volatile sig_atomic_t beep_count = 0;
volatile sig_atomic_t finished = 0;

void sigint_handler(int sig) {
    (void)sig;

    write(STDOUT_FILENO, "\a", 1);
    beep_count++;
}

void sigquit_handler(int sig) {
    (void)sig;

    finished = 1;
}

int main() {
    struct termios old_term;
    struct termios new_term;
    struct sigaction sa_int;
    struct sigaction sa_quit;

    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        printf("sigaction SIGINT error\n");
        return 1;
    }

    sa_quit.sa_handler = sigquit_handler;
    sigemptyset(&sa_quit.sa_mask);
    sa_quit.sa_flags = 0;

    if (sigaction(SIGQUIT, &sa_quit, NULL) == -1) {
        printf("sigaction SIGQUIT error\n");
        return 1;
    }
    
    if (tcgetattr(STDIN_FILENO, &old_term) == -1) {
    printf("tcgetattr error\n");
    return 1;
    }

    new_term = old_term;
    new_term.c_lflag &= ~ECHOCTL;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_term) == -1) {
    printf("tcsetattr error\n");
    return 1;
    }

    printf("Beeper started!\n");
    printf("Press Ctrl+C to beep\n");
    printf("Press Ctrl+\\ to exit\n");
    printf("Waiting for signals...\n");
    fflush(stdout);

    while (!finished) {
        pause();
    }

    printf("\nTotal beeps: %d\n", (int)beep_count);
    printf("Program finished by SIGQUIT\n");
    if (tcsetattr(STDIN_FILENO, TCSANOW, &old_term) == -1) {
    printf("tcsetattr restore error\n");
    return 1;
    }

    return 0;
}
 
