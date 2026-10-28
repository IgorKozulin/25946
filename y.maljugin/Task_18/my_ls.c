#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static void format_mode(mode_t mode, char result[11])
{
    static const mode_t bits[9] = {
        S_IRUSR, S_IWUSR, S_IXUSR,
        S_IRGRP, S_IWGRP, S_IXGRP,
        S_IROTH, S_IWOTH, S_IXOTH
    };
    static const char symbols[] = "rwxrwxrwx";

    result[0] = S_ISDIR(mode) ? 'd' : S_ISREG(mode) ? '-' : '?';
    for (int i = 0; i < 9; ++i)
        result[i + 1] = (mode & bits[i]) ? symbols[i] : '-';
    result[10] = '\0';
}

static int print_file(const char *path)
{
    struct stat st;
    if (lstat(path, &st) == -1) {
        perror(path);
        return -1;
    }

    char mode[11], size[32], date[32], owner_id[32], group_id[32];
    format_mode(st.st_mode, mode);

    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);
    snprintf(owner_id, sizeof(owner_id), "%lu", (unsigned long)st.st_uid);
    snprintf(group_id, sizeof(group_id), "%lu", (unsigned long)st.st_gid);
    const char *owner = pw ? pw->pw_name : owner_id;
    const char *group = gr ? gr->gr_name : group_id;

    size[0] = '\0';
    if (S_ISREG(st.st_mode))
        snprintf(size, sizeof(size), "%lld", (long long)st.st_size);

    time_t now = time(NULL);
    const char *date_format = (now != (time_t)-1 &&
        (st.st_mtime > now || difftime(now, st.st_mtime) > 15552000.0))
        ? "%b %e  %Y" : "%b %e %H:%M";
    struct tm *modified = localtime(&st.st_mtime);
    if (modified == NULL || strftime(date, sizeof(date), date_format, modified) == 0)
        strcpy(date, "?");

    /* basename may change its argument, so use a private copy. */
    char *copy = strdup(path);
    if (copy == NULL) {
        perror("strdup");
        return -1;
    }
    const char *name = basename(copy);
    printf("%s %3llu %-12s %-12s %12s %-13s %s\n", mode,
           (unsigned long long)st.st_nlink, owner, group, size, date, name);
    free(copy);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file-or-directory ...\n", argv[0]);
        return EXIT_FAILURE;
    }

    int result = EXIT_SUCCESS;
    for (int i = 1; i < argc; ++i)
        if (print_file(argv[i]) == -1)
            result = EXIT_FAILURE;
    return result;
}
