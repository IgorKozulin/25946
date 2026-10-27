#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static struct termios original;
static int changed;

static void restore(void)
{
    if (changed)
        tcsetattr(STDIN_FILENO, TCSANOW, &original);
}

int main(void)
{
    if (tcgetattr(STDIN_FILENO, &original) == -1) {
        fprintf(stderr, "Run this program in a terminal.\n");
        return EXIT_FAILURE;
    }
    if (atexit(restore) != 0)
        return EXIT_FAILURE;

    struct termios mode = original;
    mode.c_lflag &= ~(ICANON | ECHO | ISIG);
    mode.c_cc[VMIN] = 1;
    mode.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &mode) == -1) {
        perror("tcsetattr");
        return EXIT_FAILURE;
    }
    changed = 1;

    unsigned char answer;
    printf("Continue? (y/n): ");
    fflush(stdout);
    if (read(STDIN_FILENO, &answer, 1) != 1) {
        perror("read");
        return EXIT_FAILURE;
    }
    printf("%c\nYou answered: %c\n", answer, answer);
    return EXIT_SUCCESS;
}
