#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <libgen.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s file...\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        struct stat st;

        if (lstat(argv[i], &st) == -1) {
            perror(argv[i]);
            continue;
        }

        char rights[] = "----------";

        if (S_ISDIR(st.st_mode))
            rights[0] = 'd';
        else if (!S_ISREG(st.st_mode))
            rights[0] = '?';

        rights[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
        rights[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
        rights[3] = (st.st_mode & S_IXUSR) ? 'x' : '-';

        rights[4] = (st.st_mode & S_IRGRP) ? 'r' : '-';
        rights[5] = (st.st_mode & S_IWGRP) ? 'w' : '-';
        rights[6] = (st.st_mode & S_IXGRP) ? 'x' : '-';

        rights[7] = (st.st_mode & S_IROTH) ? 'r' : '-';
        rights[8] = (st.st_mode & S_IWOTH) ? 'w' : '-';
        rights[9] = (st.st_mode & S_IXOTH) ? 'x' : '-';

        struct passwd *pw = getpwuid(st.st_uid);
        struct group *gr = getgrgid(st.st_gid);

        char size[32] = "";

        if (S_ISREG(st.st_mode))
            snprintf(size, sizeof(size), "%lld",
                     (long long)st.st_size);

        char date[32] = "";
        struct tm *t = localtime(&st.st_mtime);

        if (t != NULL) {
            if (difftime(time(NULL), st.st_mtime) > 180 * 24 * 60 * 60)
                strftime(date, sizeof(date), "%b %e %Y", t);
            else
                strftime(date, sizeof(date), "%b %e %H:%M", t);
        }

        char *path = strdup(argv[i]);

        if (path == NULL) {
            perror("strdup");
            continue;
        }

        printf("%s %4lu %-10s %-10s %10s %s %s\n",
               rights,
               (unsigned long)st.st_nlink,
               pw ? pw->pw_name : "?",
               gr ? gr->gr_name : "?",
               size,
               date,
               basename(path));

        free(path);
    }

    return 0;
}
