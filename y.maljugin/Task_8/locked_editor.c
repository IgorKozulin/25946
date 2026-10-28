#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static int safe_editor_name(const char *name)
{
    if (*name == '\0')
        return 0;
    for (; *name != '\0'; ++name)
        if (!isalnum((unsigned char)*name) && *name != '/' &&
            *name != '_' && *name != '-' && *name != '.')
            return 0;
    return 1;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file-to-edit\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *editor = getenv("EDITOR");
    if (editor == NULL || *editor == '\0')
        editor = "vi";
    if (!safe_editor_name(editor)) {
        fprintf(stderr, "EDITOR must be one program name or path, without arguments.\n");
        return EXIT_FAILURE;
    }

    int fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }
    struct stat info;
    if (fstat(fd, &info) == -1 || !S_ISREG(info.st_mode)) {
        fprintf(stderr, "Target must be a regular file.\n");
        close(fd);
        return EXIT_FAILURE;
    }

    struct flock lock = {0};
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;             /* zero means the whole file */
    if (fcntl(fd, F_SETLK, &lock) == -1) {
        perror("fcntl lock");
        close(fd);
        return EXIT_FAILURE;
    }

    /* The shell expands the file path as data inside double quotes. */
    if (setenv("EDITOR_TARGET", argv[1], 1) == -1) {
        perror("setenv");
        close(fd);
        return EXIT_FAILURE;
    }
    size_t length = strlen(editor) + sizeof(" \"$EDITOR_TARGET\"");
    char *command = malloc(length);
    if (command == NULL) {
        perror("malloc");
        close(fd);
        return EXIT_FAILURE;
    }
    snprintf(command, length, "%s \"$EDITOR_TARGET\"", editor);
    printf("Locked %s; starting %s.\n", argv[1], editor);
    fflush(stdout);
    int status = system(command);
    free(command);

    lock.l_type = F_UNLCK;
    if (fcntl(fd, F_SETLK, &lock) == -1)
        perror("fcntl unlock");
    close(fd);
    if (status == -1) {
        perror("system");
        return EXIT_FAILURE;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0
        ? EXIT_SUCCESS : EXIT_FAILURE;
}
