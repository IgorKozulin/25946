#include <stdio.h>
#include <time.h>
#include <stdlib.h>
int main(int argc, char *argv[]) {
    if (putenv("TZ=America/Los_Angeles") != 0) perror("putenv");
    time_t a = time(NULL);
    struct tm *now = localtime(&a);
    if (now == NULL){perror("localtime"); return 1;}
    printf("%02d/%02d/%d %02d:%02d %s\n", now->tm_mon+1, now->tm_mday, (now->tm_year+1900), now->tm_hour, now->tm_min, tzname[now->tm_isdst]);
    return 0;
}