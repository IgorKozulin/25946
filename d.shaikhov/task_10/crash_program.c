#include <stdio.h>

int main() {
    int *ptr = NULL;

    printf("Program is going to crash...\n");

    *ptr = 10;

    return 0;
}
