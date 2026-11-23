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

int main(int argc, char *argv[]) {
    int sock_fd;
    int length;
    char buffer[256];
    struct sockaddr_un addr;

    const char *messages[] = {
        "Hello from client!\n",
        "This is a test message.\n",
        "Goodbye!\n"
    };

    if (argc != 2) {
        printf("Usage: %s <client_name>\n", argv[0]);
        return 1;
    }

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

    printf("[CLIENT %s] Connected to server!\n", argv[1]);

    for (int i = 0; i < 3; i++) {
        length = snprintf(buffer, sizeof(buffer), "[%s] %s", argv[1], messages[i]);

        if (length < 0 || (size_t)length >= sizeof(buffer)) {
            printf("Message is too long\n");
            close(sock_fd);
            return 1;
        }

        if (write_all(sock_fd, buffer, (size_t)length) == -1) {
            if (errno == EPIPE) {
                printf("Server closed connection\n");
            } else {
                printf("Write error\n");
            }

            close(sock_fd);
            return 1;
        }

        printf("[CLIENT %s] Sent: %s", argv[1], messages[i]);
        fflush(stdout);

        if (i < 2) {
            sleep(2);
        }
    }

    printf("[CLIENT %s] All messages sent. Closing connection...\n", argv[1]);

    close(sock_fd);

    return 0;
}

