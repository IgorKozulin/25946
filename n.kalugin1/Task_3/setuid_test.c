
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef _WIN32
typedef unsigned int uid_t;

static uid_t getuid(void)
{
    return 0;
}

static uid_t geteuid(void)
{
    return 0;
}

static int setuid(uid_t uid)
{
    (void)uid;
    return 0;
}
#endif

void print_ids(void)
{
    printf("Real UID: %lu, Effective UID: %lu\n",
           (unsigned long)getuid(), (unsigned long)geteuid());
}

void check_file(void)
{
    FILE *file;

    file = fopen("data.txt", "r");
    if (file == NULL)
        perror("fopen data.txt");
    else
    {
        printf("data.txt opened successfully\n");
        fclose(file);
    }
}

int main(void)
{
    print_ids();
    // Первая проверка проходит до сброса эффективного UID.
    check_file();

    // После setuid(getuid()) программа продолжает работу с реальным UID.
    if (setuid(getuid()) == -1)
    {
        perror("setuid");
        return 1;
    }

    print_ids();
    // Теперь повторно проверяем доступ уже без прежних привилегий.
    check_file();
    return 0;
}