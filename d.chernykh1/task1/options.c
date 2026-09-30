#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ulimit.h>
#include <sys/resource.h>
extern char **environ;
int main(int argc, char *argv[]){
    int hop;
    char dir[1024];
    char words[256];
    char *optwords[256];
    int n = 0;
    while(((hop=getopt(argc, argv, "ispuU:cC:dvV:")) != -1)){
        if (hop == '?') continue;
        words[n] = hop;
        optwords[n] = optarg;
        n++;
        if(n > 255) {fprintf(stderr, "too many operands\n"); break;}
        
    }
    for (int i = n-1; i > -1; --i){
        switch(words[i]){
            case 'p':
                printf("pid=%ld ppid=%ld pgid=%ld\n", (long)getpid(), (long)getppid(), (long)getpgrp());
                break;
            case 'i':
                printf("uid=%ld euid=%ld gid=%ld egid=%ld\n", (long)getuid(), (long)geteuid(), (long)getgid(), (long)getegid());
                break;
            case 'd':
                if (getcwd(dir, sizeof(dir)) == NULL){
                    perror("getcwd");
                }
                else printf("cwd=%s\n", dir);
                break;
            case 'v':{
                int j = 0;
                while (environ[j] != NULL){
                    printf("%s\n", environ[j]);
                    j++;
                }
                }
                break;
            case 'V':
                if (putenv(optwords[i]) != 0) perror("putenv");
                break;
            case 's':
            if (setpgid(0, 0) == -1) perror("setpgid");
            break;
            case 'u':{
                long a = ulimit(UL_GETFSIZE);
                if (a == -1) perror("ulimit");
                else printf("ulimit=%ld\n", a);
                break;
            }
            case 'U':{
                char *end;
                long op = strtol(optwords[i], &end, 10);
                if(end == optwords[i] || *end != '\0' || op < 0) {fprintf(stderr, "-U: bad value \"%s\"\n", optwords[i]); break;}
                long a = ulimit(UL_SETFSIZE, op);
                if (a == -1) perror("ulimit");
                else printf("new ulimit=%ld\n", a);
                break;
            }
            case 'c':{
                struct rlimit rl;
                if (getrlimit(RLIMIT_CORE, &rl) == -1) perror("getrlimit");
                else printf("core=%llu\n", (unsigned long long)rl.rlim_cur);
                break;
            }
            case 'C':{
                char *end;
                long op = strtol(optwords[i], &end, 10);
                struct rlimit rl;
                if(end == optwords[i] || *end != '\0' || op < 0) {fprintf(stderr, "-C: bad value \"%s\"\n", optwords[i]); break;}
                if (getrlimit(RLIMIT_CORE, &rl) == -1) perror("getrlimit");
                else{
                    rl.rlim_cur = op;
                    if(setrlimit(RLIMIT_CORE, &rl) == -1) perror("setrlimit");
                }
                break;
            }

        }
    }
    return 0;
}