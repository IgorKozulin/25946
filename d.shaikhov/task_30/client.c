#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"

int write_all(int fd, const char *buffer, size_t length) {
    size_t total = 0;

    while (total < length) {
        ssize_t written = write(fd, buffer + total, length - total);

        if (written == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        if (written == 0) {
            errno = EIO;
            return -1;
        }

        total += written;
    }

    return 0;
}

int main() {
    int sock_fd;
    struct sockaddr_un addr;

    const char *messages[] = {
        "Hello World!\n",
        "This is a Test Message!!!\n",
        "Unix domain sockets are awesome.\n"
    };

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        printf("Signal error\n");
        return 1;
    }

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sock_fd == -1) {
        printf("Socket error\n");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        printf("Connect error\n");
        close(sock_fd);
        return 1;
    }

    printf("[CLIENT] Connected to server!\n");
    printf("[CLIENT] Sending messages...\n");

    for (int i = 0; i < 3; i++) {
        printf("[CLIENT] Sending: %s", messages[i]);
        fflush(stdout);

        if (write_all(sock_fd, messages[i], strlen(messages[i])) == -1) {
            if (errno == EPIPE) {
                printf("Server closed connection\n");
            } else {
                printf("Write error\n");
            }

            close(sock_fd);
            return 1;
        }
    }

    printf("[CLIENT] All messages sent. Closing connection...\n");

    close(sock_fd);

    return 0;
}

