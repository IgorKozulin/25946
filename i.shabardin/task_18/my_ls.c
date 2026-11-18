#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <libgen.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* Превращаем тип файла и девять бит доступа в строку вроде -rw-r--r--. */
static void format_mode(mode_t mode, char text[11])
{
    const mode_t bits[9] = {
        S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP,
        S_IROTH, S_IWOTH, S_IXOTH
    };
    const char letters[3] = {'r', 'w', 'x'};
    size_t i;

    text[0] = S_ISDIR(mode) ? 'd' : S_ISREG(mode) ? '-' : '?';
    for (i = 0; i < 9; ++i) {
        text[i + 1] = (mode & bits[i]) ? letters[i % 3] : '-';
    }
    text[10] = '\0';
}

/* lstat показывает саму ссылку, а не файл, на который она ведёт. */
static int print_entry(const char *path)
{
    struct stat info;
    struct passwd *owner;
    struct group *group;
    struct tm *modified;
    time_t now;
    char mode[11], owner_text[32], group_text[32], size_text[32], date[32];
    char *copy, *name;

    if (lstat(path, &info) == -1) {
        perror(path);
        return -1;
    }
    copy = strdup(path);
    if (copy == NULL) {
        perror("strdup");
        return -1;
    }
    name = basename(copy);
    format_mode(info.st_mode, mode);

    owner = getpwuid(info.st_uid);
    if (owner != NULL) {
        snprintf(owner_text, sizeof(owner_text), "%s", owner->pw_name);
    } else {
        snprintf(owner_text, sizeof(owner_text), "%ju", (uintmax_t)info.st_uid);
    }
    group = getgrgid(info.st_gid);
    if (group != NULL) {
        snprintf(group_text, sizeof(group_text), "%s", group->gr_name);
    } else {
        snprintf(group_text, sizeof(group_text), "%ju", (uintmax_t)info.st_gid);
    }
    if (S_ISREG(info.st_mode)) {
        snprintf(size_text, sizeof(size_text), "%jd", (intmax_t)info.st_size);
    } else {
        size_text[0] = '\0';
    }

    modified = localtime(&info.st_mtime);
    if (modified == NULL) {
        perror("localtime");
        free(copy);
        return -1;
    }
    now = time(NULL);
    /* Старые и будущие даты показываем с годом, недавние — со временем. */
    if (strftime(date, sizeof(date),
                 now == (time_t)-1 || info.st_mtime > now ||
                 now - info.st_mtime > 15552000L ? "%b %e %Y" : "%b %e %H:%M",
                 modified) == 0) {
        fputs("Cannot format date\n", stderr);
        free(copy);
        return -1;
    }
    printf("%s %3ju %-12s %-12s %10s %s %s\n", mode,
           (uintmax_t)info.st_nlink, owner_text, group_text,
           size_text, date, name);
    free(copy);
    return 0;
}

int main(int argc, char **argv)
{
    int i, failed = 0;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s path [path ...]\n", argv[0]);
        return EXIT_FAILURE;
    }
    for (i = 1; i < argc; ++i) {
        if (print_entry(argv[i]) == -1) {
            failed = 1;
        }
    }
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
