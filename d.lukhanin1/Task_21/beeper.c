#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    sigset_t signals;
    uintmax_t count = 0;
    int received, error;

    if (sigemptyset(&signals) == -1 ||
        sigaddset(&signals, SIGINT) == -1 ||
        sigaddset(&signals, SIGQUIT) == -1 ||
        sigprocmask(SIG_BLOCK, &signals, NULL) == -1) {
        perror("signal mask");
        return 1;
    }
    if (puts("SIGINT (Ctrl+C): звук; SIGQUIT (Ctrl+\\): итог и выход") == EOF ||
        fflush(stdout) == EOF) {
        perror("stdout");
        return 1;
    }
    for (;;) {
        error = sigwait(&signals, &received);
        if (error != 0) {
            fprintf(stderr, "sigwait: %s\n", strerror(error));
            return 1;
        }
        if (received == SIGQUIT)
            break;
        ssize_t written;
        do {
            written = write(STDOUT_FILENO, "\a", 1);
        } while (written == -1 && errno == EINTR);
        if (written != 1) {
            perror("write");
            return 1;
        }
        if (count == UINTMAX_MAX) {
            fprintf(stderr, "signal counter overflow\n");
            return 1;
        }
        ++count;
    }
    if (printf("\nВсего прозвучало сигналов: %ju\n", count) < 0 ||
        fflush(stdout) == EOF) {
        perror("stdout");
        return 1;
    }
    return 0;
}
