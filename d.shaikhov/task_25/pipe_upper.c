#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];
    pid_t pid;
    char buffer[256];
    ssize_t bytes_read;

    if (pipe(pipefd) == -1) {
        printf("Pipe error\n");
        return 1;
    }

    pid = fork();

    if (pid == -1) {
        printf("Fork error\n");
        close(pipefd[0]);
        close(pipefd[1]);
        return 1;
    }

    if (pid == 0) {
        close(pipefd[1]);

        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            for (int i = 0; i < bytes_read; i++) {
                buffer[i] = toupper((unsigned char)buffer[i]);
            }

            printf("%.*s", (int)bytes_read, buffer);
            fflush(stdout);
        }

        if (bytes_read == -1) {
            printf("Read error\n");
        }

        close(pipefd[0]);
        exit(0);
    }

    close(pipefd[0]);

    printf("[PARENT] Pipe created successfully!\n");
    printf("[PARENT] Enter text (empty line or Ctrl+D to exit):\n");

    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }

        if (buffer[0] == '\n') {
            break;
        }

        if (write(pipefd[1], buffer, strlen(buffer)) == -1) {
            printf("Write error\n");
            break;
        }
    }

    close(pipefd[1]);
    wait(NULL);

    printf("[PARENT] Child finished. Program terminated.\n");

    return 0;
}

