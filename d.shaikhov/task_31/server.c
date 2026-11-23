#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>

#define SOCKET_PATH "./socket"
#define MAX_CLIENTS 10

volatile sig_atomic_t stopped = 0;

void sigint_handler(int sig) {
    (void)sig;
    stopped = 1;
}

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
    int clients = 0;
    int slot;
    int ready;
    int disconnect;
    int result = 0;

    struct sockaddr_un addr;
    struct pollfd fds[MAX_CLIENTS + 1];
    struct sigaction sa;

    nfds_t nfds = 1;
    char buffer[256];
    ssize_t bytes_read;

    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        printf("Sigaction error\n");
        return 1;
    }

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

    for (int i = 0; i <= MAX_CLIENTS; i++) {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
        fds[i].revents = 0;
    }

    fds[0].fd = server_fd;

    printf("[SERVER] Waiting for clients on %s...\n", SOCKET_PATH);
    printf("[SERVER] Press Ctrl+C to stop.\n");
    fflush(stdout);

    while (!stopped) {
        ready = poll(fds, nfds, 1000);

        if (ready == -1) {
            if (errno == EINTR) {
                continue;
            }

            printf("Poll error\n");
            result = 1;
            break;
        }

        if (ready == 0) {
            continue;
        }

        if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            printf("Server socket error\n");
            result = 1;
            break;
        }

        if (fds[0].revents & POLLIN) {
            client_fd = accept(server_fd, NULL, NULL);

            if (client_fd == -1) {
                if (errno != EINTR) {
                    printf("Accept error\n");
                }
            } else {
                slot = -1;

                for (int i = 1; i <= MAX_CLIENTS; i++) {
                    if (fds[i].fd == -1) {
                        slot = i;
                        break;
                    }
                }

                if (slot == -1) {
                    printf("[SERVER] Too many clients!\n");
                    close(client_fd);
                } else {
                    fds[slot].fd = client_fd;
                    fds[slot].events = POLLIN;
                    fds[slot].revents = 0;

                    if ((nfds_t)(slot + 1) > nfds) {
                        nfds = (nfds_t)(slot + 1);
                    }

                    clients++;

                    printf("[SERVER] Client connected (fd=%d). Total: %d\n", client_fd, clients);
                }
            }
        }

        for (nfds_t i = 1; i < nfds; i++) {
            if (fds[i].fd == -1) {
                continue;
            }

            if (!(fds[i].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL))) {
                continue;
            }

            disconnect = 0;

            if (fds[i].revents & POLLNVAL) {
                printf("[SERVER] Invalid client descriptor: %d\n", fds[i].fd);
                disconnect = 1;
            } else {
                bytes_read = read(fds[i].fd, buffer, sizeof(buffer));

                if (bytes_read > 0) {
                    for (ssize_t j = 0; j < bytes_read; j++) {
                        buffer[j] = toupper((unsigned char)buffer[j]);
                    }

                    printf("[Client %d]: ", fds[i].fd);
                    fflush(stdout);

                    if (write_all(STDOUT_FILENO, buffer, (size_t)bytes_read) == -1) {
                        printf("\nWrite error\n");
                        result = 1;
                        stopped = 1;
                        break;
                    }
                } else if (bytes_read == 0) {
                    printf("[SERVER] Client %d disconnected.\n", fds[i].fd);
                    disconnect = 1;
                } else if (errno != EINTR) {
                    printf("[SERVER] Read error for client %d\n", fds[i].fd);
                    disconnect = 1;
                }
            }

            if (disconnect) {
                close(fds[i].fd);
                fds[i].fd = -1;
                fds[i].revents = 0;
                clients--;

                printf("[SERVER] Clients remaining: %d\n", clients);
            }
        }

        while (nfds > 1 && fds[nfds - 1].fd == -1) {
            nfds--;
        }
    }

    for (int i = 1; i <= MAX_CLIENTS; i++) {
        if (fds[i].fd != -1) {
            close(fds[i].fd);
        }
    }

    close(server_fd);
    unlink(SOCKET_PATH);

    printf("\n[SERVER] Server stopped.\n");

    return result;
}

