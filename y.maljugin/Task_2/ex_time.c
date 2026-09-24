#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main()
{
    time_t now;
    struct tm *sp;

    if (setenv("TZ", "America/Los_Angeles", 1) == -1) {
        perror("setenv");
        return 1;
    }
    tzset();

    if (time(&now) == (time_t)-1) {
        perror("time");
        return 1;
    }
    sp = localtime(&now);
    if (sp == NULL) {
        perror("localtime");
        return 1;
    }

    printf("%02d/%02d/%04d %02d:%02d:%02d %s\n",
           sp->tm_mon + 1,
           sp->tm_mday,
           sp->tm_year + 1900,
           sp->tm_hour,
           sp->tm_min,
           sp->tm_sec,
           tzname[sp->tm_isdst > 0 ? 1 : 0]);

    return 0;
}
