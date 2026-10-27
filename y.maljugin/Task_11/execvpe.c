#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Run an executable script with /bin/sh, as execvp does on ENOEXEC. */
static int run_as_script(const char *path, char *const argv[], char *const envp[])
{
    size_t count = 0;
    while (argv[count] != NULL)
        ++count;
    char **shell_args = calloc(count + 2, sizeof(*shell_args));
    if (shell_args == NULL)
        return -1;
    shell_args[0] = (char *)"sh";
    shell_args[1] = (char *)path;
    for (size_t i = 1; i < count; ++i)
        shell_args[i + 1] = argv[i];
    execve("/bin/sh", shell_args, envp);
    int error = errno;
    free(shell_args);
    errno = error;
    return -1;
}

int execvpe(const char *file, char *const argv[], char *const envp[])
{
    if (file == NULL || *file == '\0') {
        errno = ENOENT;
        return -1;
    }
    if (strchr(file, '/') != NULL) {
        execve(file, argv, envp);
        if (errno == ENOEXEC)
            return run_as_script(file, argv, envp);
        return -1;
    }

    /* getenv reads the caller's environment; envp is passed to the child. */
    const char *path = getenv("PATH");
    if (path == NULL)
        path = "/usr/bin:/bin";
    size_t file_length = strlen(file);
    int permission_denied = 0;

    for (;;) {
        const char *separator = strchr(path, ':');
        size_t directory_length = separator ? (size_t)(separator - path) : strlen(path);
        size_t candidate_length = directory_length + (directory_length ? 1 : 0)
                                + file_length + 1;
        char *candidate = malloc(candidate_length);
        if (candidate == NULL)
            return -1;
        if (directory_length != 0) {
            memcpy(candidate, path, directory_length);
            candidate[directory_length] = '/';
        }
        strcpy(candidate + directory_length + (directory_length ? 1 : 0), file);

        execve(candidate, argv, envp);
        int error = errno;
        if (error == ENOEXEC) {
            run_as_script(candidate, argv, envp);
            error = errno;
            free(candidate);
            errno = error;
            return -1;
        }
        free(candidate);
        if (error == EACCES)
            permission_denied = 1;
        else if (error != ENOENT && error != ENOTDIR) {
            errno = error;
            return -1;
        }
        if (separator == NULL)
            break;
        path = separator + 1;
    }
    errno = permission_denied ? EACCES : ENOENT;
    return -1;
}
