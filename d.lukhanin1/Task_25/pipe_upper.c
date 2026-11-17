#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static int write_all(int fd, const unsigned char *buffer, size_t size)
{
    size_t done = 0;
    while (done < size) {
        ssize_t count = write(fd, buffer + done, size - done);
        if (count == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("write");
            return -1;
        }
        if (count == 0) {
            fprintf(stderr, "write: no progress\n");
            return -1;
        }
        done += (size_t)count;
    }
    return 0;
}

static int transfer(int input, int output, int uppercase)
{
    unsigned char buffer[4096];
    for (;;) {
        ssize_t count = read(input, buffer, sizeof(buffer));
        if (count == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("read");
            return -1;
        }
        if (count == 0) {
            return 0;
        }
        if (uppercase) {
            for (ssize_t i = 0; i < count; ++i) {
                buffer[i] = (unsigned char)toupper((unsigned char)buffer[i]);
            }
        }
        if (write_all(output, buffer, (size_t)count) == -1) {
            return -1;
        }
    }
}

int main(void)
{
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        return EXIT_FAILURE;
    }
    int channel[2];
    if (pipe(channel) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }
    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        close(channel[0]);
        close(channel[1]);
        return EXIT_FAILURE;
    }
    if (child == 0) {
        close(channel[1]);
        int result = transfer(channel[0], STDOUT_FILENO, 1);
        close(channel[0]);
        _exit(result == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    close(channel[0]);
    int failed = transfer(STDIN_FILENO, channel[1], 0) == -1;
    if (close(channel[1]) == -1) {
        perror("close");
        failed = 1;
    }
    int status;
    while (waitpid(child, &status, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            return EXIT_FAILURE;
        }
    }
    return !failed && WIFEXITED(status) && WEXITSTATUS(status) == 0
        ? EXIT_SUCCESS : EXIT_FAILURE;
}
