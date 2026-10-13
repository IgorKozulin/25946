#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void)
{
    time_t now;
    struct tm *california;
    const char *zone;

    if (setenv("TZ", "America/Los_Angeles", 1) != 0) {
        perror("setenv");
        return EXIT_FAILURE;
    }
    tzset();

    now = time(NULL);
    if (now == (time_t)-1) {
        perror("time");
        return EXIT_FAILURE;
    }
    california = localtime(&now);
    if (california == NULL) {
        perror("localtime");
        return EXIT_FAILURE;
    }

    zone = tzname[california->tm_isdst > 0 ? 1 : 0];
    printf("%02d/%02d/%04d %02d:%02d %s\n",
           california->tm_mon + 1, california->tm_mday,
           california->tm_year + 1900, california->tm_hour,
           california->tm_min, zone);
    return EXIT_SUCCESS;
}
