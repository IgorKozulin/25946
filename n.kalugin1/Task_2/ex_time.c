#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <IANA_timezone>\n", argv[0]);
        fprintf(stderr, "Example: %s Europe/Moscow\n", argv[0]);
        return 1;
    }

    const char *tz = argv[1];

#ifdef _WIN32
    if (_putenv_s("TZ", tz) != 0) {
        fprintf(stderr, "Unable to set TZ to %s\n", tz);
        return 1;
    }
    _tzset();
#else
    if (setenv("TZ", tz, 1) == -1) {
        perror("setenv");
        return 1;
    }
    tzset();
#endif

    time_t now = time(NULL);    //возвращает число секунд, прошедших с 01.01.1970
    if (now == (time_t)-1) { perror("time"); return 1; }

    struct tm *lt = localtime(&now);    //число преобразуется во время
    if (!lt) { perror("localtime"); return 1; }

    char zone_abbr[16] = {0};    // Буфер для хранения аббревиатуры часового пояса  
    strftime(zone_abbr, sizeof(zone_abbr), "%Z", lt);

    printf("%s: %02d/%02d/%04d %02d:%02d:%02d %s\n",
           tz,
           lt->tm_mon + 1, lt->tm_mday, lt->tm_year + 1900,
           lt->tm_hour, lt->tm_min, lt->tm_sec,
           zone_abbr);

    return 0;
}