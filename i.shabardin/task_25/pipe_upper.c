#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* write может записать только часть буфера; отправляем остаток в цикле. */
static int write_all(int fd, const char *text, size_t length)
{
    size_t sent = 0;

    while (sent < length) {
        ssize_t count = write(fd, text + sent, length - sent);
        if (count == -1 && errno == EINTR) {
            continue;
        }
        if (count <= 0) {
            perror("write");
            return -1;
        }
        sent += (size_t)count;
    }
    return 0;
}

/* Потомок читает канал до EOF и переводит каждый полученный байт в верхний регистр. */
static int read_and_print(int fd)
{
    char buffer[256];

    for (;;) {
        ssize_t count = read(fd, buffer, sizeof(buffer));
        ssize_t i;

        if (count == -1 && errno == EINTR) {
            continue;
        }
        if (count == -1) {
            perror("read");
            return -1;
        }
        if (count == 0) {
            return 0;
        }
        for (i = 0; i < count; ++i) {
            buffer[i] = (char)toupper((unsigned char)buffer[i]);
        }
        if (write_all(STDOUT_FILENO, buffer, (size_t)count) == -1) {
            return -1;
        }
    }
}

int main(void)
{
    int channel[2], status, failed = 0;
    pid_t child;
    char line[256];

    if (pipe(channel) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }
    child = fork();
    if (child == -1) {
        perror("fork");
        close(channel[0]);
        close(channel[1]);
        return EXIT_FAILURE;
    }
    if (child == 0) {
        close(channel[1]); /* Без этого read не увидит EOF. */
        failed = read_and_print(channel[0]) == -1;
        close(channel[0]);
        _exit(failed ? EXIT_FAILURE : EXIT_SUCCESS);
    }

    close(channel[0]);
    while (1) {
        if (isatty(STDIN_FILENO)) {
            fputs("> ", stderr);
            fflush(stderr);
        }
        if (fgets(line, sizeof(line), stdin) == NULL) {
            if (ferror(stdin)) {
                perror("fgets");
                failed = 1;
            }
            break;
        }
        if (strcmp(line, "\n") == 0 || strcmp(line, "\r\n") == 0) {
            break;
        }
        if (write_all(channel[1], line, strlen(line)) == -1) {
            failed = 1;
            break;
        }
    }
    /* Закрытие записи сообщает потомку, что больше текста не будет. */
    if (close(channel[1]) == -1) {
        perror("close");
        failed = 1;
    }
    while (waitpid(child, &status, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            return EXIT_FAILURE;
        }
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        failed = 1;
    }
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
