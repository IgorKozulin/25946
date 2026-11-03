#include <stdio.h>
#include <time.h>
#include <stdlib.h>
void print_california_time(void){
    if (putenv("TZ=America/Los_Angeles") != 0) {
        perror("putenv");
        return;
    }
    tzset();
    time_t a = time(NULL);
    if (a == (time_t)-1) {
        perror("time");
        return;
    }
    struct tm *now = localtime(&a);
    if (now == NULL){perror("localtime"); return;}
    printf("%02d/%02d/%d %02d:%02d %s\n", now->tm_mon+1, now->tm_mday, (now->tm_year+1900), now->tm_hour, now->tm_min, tzname[now->tm_isdst]);
    return;
}
