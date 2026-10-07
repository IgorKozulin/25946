#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    setenv("TZ", "America/Los_Angeles", 1);
    tzset();
    time_t now = time(NULL);
    struct tm *sp = localtime(&now);
    printf("%02d/%02d/%04d %02d:%02d %s\n",
           sp->tm_mday,
           sp->tm_mon + 1,
           sp->tm_year + 1900,
           sp->tm_hour,
           sp->tm_min,
           tzname[sp->tm_isdst]);
    return 0;
}
