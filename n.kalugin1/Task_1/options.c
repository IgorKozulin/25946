#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ulimit.h>
#include <sys/resource.h>
#include <string.h>

#define MAXOPTS 100

extern char **environ;

//Выполняет одну опцию c со значением arg
void do_option(int c, char *arg)
{
    char buf[1024];
    long newlim;
    char *end;
    char *equal_sign;
    char *name;
    size_t name_length;
    struct rlimit rl;
    char **env;

    switch (c) {
    case 'i':
        printf("uid=%d euid=%d gid=%d egid=%d\n",
               (int)getuid(), (int)geteuid(),
               (int)getgid(), (int)getegid());
        break;
    case 's':
        if (setpgid(0, 0) == -1)
            perror("setpgid");
        else
            printf("процесс стал лидером группы\n");
        break;
    case 'p':
        printf("pid=%d ppid=%d pgid=%d\n",
               (int)getpid(), (int)getppid(), (int)getpgrp());
        break;
    case 'u':
        printf("ulimit=%ld\n", ulimit(UL_GETFSIZE));
        break;
    case 'U':
        newlim = strtol(arg, &end, 10);
        if (*end != '\0' || newlim < 0) {
            printf("плохое значение для -U: %s\n", arg);
            break;
        }
        if (ulimit(UL_SETFSIZE, newlim) == -1)
            perror("ulimit");
        break;
    case 'c':
        if (getrlimit(RLIMIT_CORE, &rl) == -1)
            perror("getrlimit");
        else if (rl.rlim_cur == RLIM_INFINITY)
            printf("core limit=unlimited\n");
        else
            printf("core limit=%ld\n", (long)rl.rlim_cur);
        break;
    case 'C':
        newlim = strtol(arg, &end, 10);
        if (*end != '\0' || newlim < 0) {
            printf("плохое значение для -C: %s\n", arg);
            break;
        }
        if (getrlimit(RLIMIT_CORE, &rl) == -1) {
            perror("getrlimit");
            break;
        }
        rl.rlim_cur = newlim;
        if (setrlimit(RLIMIT_CORE, &rl) == -1)
            perror("setrlimit");
        break;
    case 'd':
        if (getcwd(buf, sizeof(buf)) != NULL)
            printf("cwd=%s\n", buf);
        else
            perror("getcwd");
        break;
    case 'v':
        for (env = environ; *env != NULL; env++)
            printf("%s\n", *env);
        break;
    case 'V':
        equal_sign = strchr(arg, '=');  //ищем символ '=' в строке arg
        if (equal_sign == NULL || equal_sign == arg) {
            printf("плохое значение для -V: %s (нужно имя=значение)\n", arg);
            break;
        }

        name_length = (size_t)(equal_sign - arg);   //вычисляем длину имени переменной
        name = (char *)malloc(name_length + 1);     //выделяем память для имени переменной
        if (name == NULL) {
            perror("malloc");      
            break;
        }

        memcpy(name, arg, name_length); //копируем имя переменной в выделенную память
        name[name_length] = '\0';   //добавляем завершающий нулевой символ
        if (setenv(name, equal_sign + 1, 1) != 0)   
            perror("setenv");
        free(name);
        break;
    }
}

int main(int argc, char *argv[])  
{
    int c;
    int n = 0;
    int i;
    int opts[MAXOPTS];
    char *args[MAXOPTS];

    if (argc == 1) {
        printf("Использование: %s [-i] [-s] [-p] [-u] [-Uзначение] [-c] [-Cзначение] [-d] [-v] [-Vимя=значение]\n", argv[0]);
        return 0;
    }

    /* Проход 1: собираем опции слева направо */
    while ((c = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {
        if (c == '?') {
            printf("неизвестная опция\n");
            continue;
        }
        if (n >= MAXOPTS) {
            printf("слишком много опций\n");
            return 1;
        }
        opts[n] = c;
        args[n] = optarg;
        n++;
    }

    /* Проход 2: выполняем справа налево */
    for (i = n - 1; i >= 0; i--) {
        do_option(opts[i], args[i]);
    }
    return 0;
}