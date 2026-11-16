#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <libgen.h>

void make_permissions(mode_t mode, char permissions[11]) {
    permissions[0] = S_ISDIR(mode) ? 'd' : (S_ISREG(mode) ? '-' : '?');
    permissions[1] = (mode & S_IRUSR) ? 'r' : '-';
    permissions[2] = (mode & S_IWUSR) ? 'w' : '-';
    permissions[3] = (mode & S_IXUSR) ? 'x' : '-';
    permissions[4] = (mode & S_IRGRP) ? 'r' : '-';
    permissions[5] = (mode & S_IWGRP) ? 'w' : '-';
    permissions[6] = (mode & S_IXGRP) ? 'x' : '-';
    permissions[7] = (mode & S_IROTH) ? 'r' : '-';
    permissions[8] = (mode & S_IWOTH) ? 'w' : '-';
    permissions[9] = (mode & S_IXOTH) ? 'x' : '-';
    permissions[10] = '\0';
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s file1 [file2 ...]\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        struct stat st;
        char permissions[11];
        char date[32];
        struct passwd *user;
        struct group *group;
        struct tm *file_time;
        time_t now;
        char *path_copy;
        char *name;

        if (lstat(argv[i], &st) == -1) {
            perror(argv[i]);
            continue;
        }

        make_permissions(st.st_mode, permissions);
        user = getpwuid(st.st_uid);
        group = getgrgid(st.st_gid);

        now = time(NULL);
        file_time = localtime(&st.st_mtime);

        if (file_time == NULL) {
            strcpy(date, "?");
        } else if (difftime(now, st.st_mtime) > 180.0 * 24 * 60 * 60) {
            strftime(date, sizeof(date), "%b %e %Y", file_time);
        } else {
            strftime(date, sizeof(date), "%b %e %H:%M", file_time);
        }

        path_copy = malloc(strlen(argv[i]) + 1);
        if (path_copy == NULL) {
            printf("Memory allocation error\n");
            continue;
        }

        strcpy(path_copy, argv[i]);
        name = basename(path_copy);

        printf("%-10s %4lu %-8s %-8s ", permissions, (unsigned long)st.st_nlink, user != NULL ? user->pw_name : "?", group != NULL ? group->gr_name : "?");

        if (S_ISREG(st.st_mode)) {
            printf("%10lld ", (long long)st.st_size);
        } else {
            printf("%10s ", "");
        }

        printf("%-12s %s\n", date, name);

        free(path_copy);
    }

    return 0;
}
