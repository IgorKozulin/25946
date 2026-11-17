#include <errno.h>
#include <grp.h>
#include <inttypes.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static int print_file(const char *argument)
{
    char *path = strdup(argument);
    struct stat st;
    char mode[11];
    const mode_t bits[] = {S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP,
                           S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH};
    char owner_number[32], group_number[32], size[32] = "";
    char date[32];
    struct tm tm;
    const char *name, *owner, *group;
    struct passwd *pw;
    struct group *gr;
    size_t length;

    if (path == NULL) {
        perror("strdup");
        return 1;
    }
    length = strlen(path);
    while (length > 1 && path[length - 1] == '/')
        path[--length] = '\0';
    if (lstat(path, &st) == -1) {
        perror(argument);
        free(path);
        return 1;
    }
    mode[0] = S_ISDIR(st.st_mode) ? 'd' : S_ISREG(st.st_mode) ? '-' : '?';
    for (size_t i = 0; i < 9; ++i)
        mode[i + 1] = st.st_mode & bits[i] ? "rwx"[i % 3] : '-';
    mode[10] = '\0';
    snprintf(owner_number, sizeof owner_number, "%ju", (uintmax_t)st.st_uid);
    snprintf(group_number, sizeof group_number, "%ju", (uintmax_t)st.st_gid);
    pw = getpwuid(st.st_uid);
    owner = pw == NULL ? owner_number : pw->pw_name;
    gr = getgrgid(st.st_gid);
    group = gr == NULL ? group_number : gr->gr_name;
    if (S_ISREG(st.st_mode))
        snprintf(size, sizeof size, "%jd", (intmax_t)st.st_size);
    if (localtime_r(&st.st_mtime, &tm) == NULL ||
        strftime(date, sizeof date, "%Y-%m-%d %H:%M", &tm) == 0) {
        fprintf(stderr, "%s: cannot format modification time\n", argument);
        free(path);
        return 1;
    }
    name = strrchr(path, '/');
    name = name == NULL ? path : name[1] == '\0' ? "/" : name + 1;
    printf("%s %5ju %-16s %-16s %12s %s %s\n", mode,
           (uintmax_t)st.st_nlink, owner, group, size, date, name);
    free(path);
    return 0;
}

int main(int argc, char **argv)
{
    int status = 0;

    if (argc == 1)
        status = print_file(".");
    for (int i = 1; i < argc; ++i)
        if (print_file(argv[i]) != 0)
            status = 1;
    if (fflush(stdout) == EOF) {
        perror("stdout");
        status = 1;
    }
    return status;
}
