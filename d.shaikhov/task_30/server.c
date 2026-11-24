#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
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
            return -1;
        }

        total += written;
    }

    return 0;
}

int main() {
    int server_fd;
    int client_fd;
    struct sockaddr_un addr;
    char buffer[256];
    ssize_t bytes_read;
    int result = 0;

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (server_fd == -1) {
        printf("Socket error\n");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    unlink(SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        printf("Bind error\n");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == -1) {
        printf("Listen error\n");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    printf("[SERVER] Waiting for client on %s...\n", SOCKET_PATH);
    fflush(stdout);

    client_fd = accept(server_fd, NULL, NULL);

    if (client_fd == -1) {
        printf("Accept error\n");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    printf("[SERVER] Client connected!\n");
    fflush(stdout);

    while (1) {
        bytes_read = read(client_fd, buffer, sizeof(buffer));

        if (bytes_read == 0) {
            break;
        }

        if (bytes_read == -1) {
            if (errno == EINTR) {
                continue;
            }
            printf("Read error\n");
            result = 1;
            break;
        }

        for (ssize_t i = 0; i < bytes_read; i++) {
            buffer[i] = toupper((unsigned char)buffer[i]);
        }

        if (write_all(STDOUT_FILENO, buffer, (size_t)bytes_read) == -1) {
            printf("Write error\n");
            result = 1;
            break;
        }
    }

    if (bytes_read == 0) {
        printf("\n[SERVER] Client disconnected.\n");
    }

    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);

    return result;
}

