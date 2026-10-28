#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// выводим реальный и эффективный uid текущего процесса
static void print_uids(void)
{
    printf("real_uid=%ld\n", (long)getuid());
    printf("effective_uid=%ld\n", (long)geteuid());
}

// показываем текущее время в калифорнии
static int print_california_time(void)
{
    time_t now;
    struct tm *local;
    const char *zone;

    // задаем часовой пояс для текущего процесса
    if (setenv("TZ", "America/Los_Angeles", 1) == -1) {
        perror("setenv");
        return -1;
    }
    tzset();

    // получаем время и переводим его в выбранный пояс
    if (time(&now) == (time_t)-1) {
        fprintf(stderr, "time: cannot get current time\n");
        return -1;
    }

    local = localtime(&now);
    if (local == NULL) {
        perror("localtime");
        return -1;
    }

    // зимой будет pst, летом pdt
    zone = tzname[local->tm_isdst > 0 ? 1 : 0];

    printf("california_time=%02d/%02d/%04d %02d:%02d %s\n",
           local->tm_mon + 1,
           local->tm_mday,
           local->tm_year + 1900,
           local->tm_hour,
           local->tm_min,
           zone);

    return 0;
}

// пробуем открыть файл и при успехе показываем время доступа
static void check_file_access(const char *filename)
{
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        perror("fopen");
        printf("file_access=denied\n");
        return;
    }

    printf("file_access=granted (%s)\n", filename);
    print_california_time();
    fclose(file);
}

int main(void)
{
    // первый этап: проверяем пользователя и доступ к файлу
    printf("=== before setuid ===\n");
    print_uids();
    check_file_access("data.txt");

    // второй этап: пробуем установить реальный uid процесса
    if (setuid(getuid()) == -1) {
        perror("setuid");
        return EXIT_FAILURE;
    }

    // третий этап: повторяем проверку после setuid
    printf("\n=== after setuid ===\n");
    print_uids();
    check_file_access("data.txt");

    return EXIT_SUCCESS;
}

// сборка: make
// запуск: ./access_time
// очистка: make clean
