#define _POSIX_C_SOURCE 200809L
#define _FILE_OFFSET_BITS 64

#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

enum { SIX_MONTHS_IN_SECONDS = 180 * 24 * 60 * 60 };

static void format_mode(mode_t mode, char text[11])
{
    static const mode_t bits[] = {
        S_IRUSR, S_IWUSR, S_IXUSR,
        S_IRGRP, S_IWGRP, S_IXGRP,
        S_IROTH, S_IWOTH, S_IXOTH
    };
    static const char permissions[] = "rwxrwxrwx";
    size_t i;

    text[0] = '?';
    if (S_ISDIR(mode)) {
        text[0] = 'd';
    } else if (S_ISREG(mode)) {
        text[0] = '-';
    }

    for (i = 0; i < sizeof(bits) / sizeof(bits[0]); ++i) {
        text[i + 1] = (mode & bits[i]) ? permissions[i] : '-';
    }
    text[i + 1] = '\0';
}

static const char *format_mtime(time_t mtime, time_t now, char *date, size_t size)
{
    const struct tm *local_mtime = localtime(&mtime);
    const char *format = "%b %e %H:%M";

    if (now != (time_t)-1 && difftime(now, mtime) > SIX_MONTHS_IN_SECONDS) {
        format = "%b %e %Y";
    }
    if (local_mtime == NULL || strftime(date, size, format, local_mtime) == 0) {
        return "?";
    }
    return date;
}

static int list_path(char *path, time_t now)
{
    struct stat metadata;
    const struct passwd *owner;
    const struct group *group;
    char mode[11];
    char date[64];

    if (lstat(path, &metadata) == -1) {
        perror(path);
        return -1;
    }

    format_mode(metadata.st_mode, mode);
    owner = getpwuid(metadata.st_uid);
    group = getgrgid(metadata.st_gid);
    printf("%s %4ju %-10s %-10s ", mode, (uintmax_t)metadata.st_nlink,
           owner ? owner->pw_name : "?", group ? group->gr_name : "?");

    if (S_ISREG(metadata.st_mode)) {
        printf("%10jd ", (intmax_t)metadata.st_size);
    } else {
        printf("%10s ", "");
    }

    printf("%-12s %s\n", format_mtime(metadata.st_mtime, now, date, sizeof(date)),
           basename(path));
    return 0;
}

int main(int argc, char *argv[])
{
    int status = EXIT_SUCCESS;
    int i;
    time_t now;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s path [path ...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    now = time(NULL);
    for (i = 1; i < argc; ++i) {
        if (list_path(argv[i], now) == -1) {
            status = EXIT_FAILURE;
        }
    }
    if (fflush(stdout) == EOF || ferror(stdout)) {
        perror("stdout");
        status = EXIT_FAILURE;
    }
    return status;
}
