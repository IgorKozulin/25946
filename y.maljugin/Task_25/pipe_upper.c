#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static int write_all(int fd, const char *data, size_t length)
{
    while (length > 0) {
        ssize_t sent = write(fd, data, length);
        if (sent == -1 && errno == EINTR)
            continue;
        if (sent <= 0)
            return -1;
        data += sent;
        length -= (size_t)sent;
    }
    return 0;
}

int main(void)
{
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

    char buffer[4096];
    if (child == 0) {
        close(channel[1]);
        ssize_t received;
        while ((received = read(channel[0], buffer, sizeof(buffer))) > 0) {
            for (ssize_t i = 0; i < received; ++i)
                buffer[i] = (char)toupper((unsigned char)buffer[i]);
            if (write_all(STDOUT_FILENO, buffer, (size_t)received) == -1) {
                perror("write stdout");
                _exit(EXIT_FAILURE);
            }
        }
        if (received == -1)
            perror("read pipe");
        close(channel[0]);
        _exit(received == -1 ? EXIT_FAILURE : EXIT_SUCCESS);
    }

    close(channel[0]);
    ssize_t received;
    int result = EXIT_SUCCESS;
    while ((received = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
        if (write_all(channel[1], buffer, (size_t)received) == -1) {
            perror("write pipe");
            result = EXIT_FAILURE;
            break;
        }
    }
    if (received == -1) {
        perror("read stdin");
        result = EXIT_FAILURE;
    }
    close(channel[1]);

    int status;
    while (waitpid(child, &status, 0) == -1) {
        if (errno == EINTR)
            continue;
        perror("waitpid");
        return EXIT_FAILURE;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS)
        result = EXIT_FAILURE;
    return result;
}
