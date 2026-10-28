#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <libgen.h>
#include <errno.h>

static void mode_string(mode_t mode, char out[11]) {
    if (S_ISDIR(mode))       out[0] = 'd';
    else if (S_ISREG(mode))  out[0] = '-';
    else if (S_ISLNK(mode))  out[0] = 'l';
    else if (S_ISCHR(mode))  out[0] = 'c';
    else if (S_ISBLK(mode))  out[0] = 'b';
    else if (S_ISFIFO(mode)) out[0] = 'p';
    else if (S_ISSOCK(mode)) out[0] = 's';
    else                     out[0] = '?';

    out[1] = (mode & S_IRUSR) ? 'r' : '-';
    out[2] = (mode & S_IWUSR) ? 'w' : '-';
    out[3] = (mode & S_IXUSR) ? 'x' : '-';
    out[4] = (mode & S_IRGRP) ? 'r' : '-';
    out[5] = (mode & S_IWGRP) ? 'w' : '-';
    out[6] = (mode & S_IXGRP) ? 'x' : '-';
    out[7] = (mode & S_IROTH) ? 'r' : '-';
    out[8] = (mode & S_IWOTH) ? 'w' : '-';
    out[9] = (mode & S_IXOTH) ? 'x' : '-';
    out[10] = '\0';
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s file...\n", argv[0]);
        return 1;
    }

    time_t now = time(NULL);

    for (int i = 1; i < argc; i++) {
        struct stat st;
        if (lstat(argv[i], &st) == -1) {
            perror(argv[i]);
            continue;
        }

        char modes[11];
        mode_string(st.st_mode, modes);

        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);
        const char *uname = pw ? pw->pw_name : "?";
        const char *gname = gr ? gr->gr_name : "?";

        char datebuf[64];
        struct tm *tm = localtime(&st.st_mtime);
        if (tm) {
            double diff = difftime(now, st.st_mtime);
            const char *fmt = (diff > 15552000.0 || diff < 0)
                              ? "%b %e  %Y"
                              : "%b %e %H:%M";
            strftime(datebuf, sizeof(datebuf), fmt, tm);
        } else {
            strcpy(datebuf, "?");
        }

        char sizebuf[32];
        if (S_ISREG(st.st_mode))
            snprintf(sizebuf, sizeof(sizebuf), "%lld", (long long)st.st_size);
        else
            sizebuf[0] = '\0';

        char pathcopy[4096];
        strncpy(pathcopy, argv[i], sizeof(pathcopy) - 1);
        pathcopy[sizeof(pathcopy) - 1] = '\0';
        const char *name = basename(pathcopy);

        printf("%s %4ld %-8s %-8s %8s %s %s\n",
               modes,
               (long)st.st_nlink,
               uname,
               gname,
               sizebuf,
               datebuf,
               name);
    }

    return 0;
}
