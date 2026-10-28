#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <errno.h>

#define MAX_BUF 4096
#define LINE_WIDTH 40
#define PROMPT "> "

static struct termios orig_termios;
static int raw_enabled = 0;
static unsigned char erase_char;
static unsigned char kill_char;
static char buffer[MAX_BUF];
static int pos = 0;

static void disable_raw_mode(void) {
    if (raw_enabled) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        raw_enabled = 0;
    }
}

static void die(const char *msg) {
    disable_raw_mode();
    perror(msg);
    exit(1);
}

static void enable_raw_mode(void) {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
        die("tcgetattr");

    erase_char = orig_termios.c_cc[VERASE];
    kill_char  = orig_termios.c_cc[VKILL];

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr");

    raw_enabled = 1;
    atexit(disable_raw_mode);
}

static void redraw(void) {
    printf("\r\033[K");

    int col = 0;
    int i = 0;

    while (i < pos) {
        if (buffer[i] == ' ') {
            if (col + 1 > LINE_WIDTH) {
                printf("\n\033[K");
                col = 0;
            }
            putchar(' ');
            col++;
            i++;
            continue;
        }

        int start = i;
        while (i < pos && buffer[i] != ' ') i++;
        int wlen = i - start;

        if (wlen > LINE_WIDTH) {
            for (int k = 0; k < wlen; k++) {
                if (col == LINE_WIDTH) {
                    printf("\n\033[K");
                    col = 0;
                }
                putchar(buffer[start + k]);
                col++;
            }
        } else {
            if (col > 0 && col + wlen > LINE_WIDTH) {
                printf("\n\033[K");
                col = 0;
            }
            for (int k = 0; k < wlen; k++) {
                putchar(buffer[start + k]);
                col++;
            }
        }
    }

    fflush(stdout);
}

static void delete_word(void) {
    int i = pos;
    while (i > 0 && buffer[i - 1] == ' ') i--;
    while (i > 0 && buffer[i - 1] != ' ') i--;
    pos = i;
}

int main(void) {
    enable_raw_mode();

    printf("%s", PROMPT);
    fflush(stdout);

    char c;
    while (1) {
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n == -1) {
            if (errno == EINTR) continue;
            die("read");
        }
        if (n == 0) break;

        unsigned char uc = (unsigned char)c;

        if (uc == 4) {
            if (pos == 0) {
                printf("\nExiting...\n");
                disable_raw_mode();
                return 0;
            } else {
                putchar('\a');
                fflush(stdout);
                continue;
            }
        }

        if (uc == '\n' || uc == '\r') {
            printf("\n");
            pos = 0;
            printf("%s", PROMPT);
            fflush(stdout);
            continue;
        }

        if (uc == erase_char) {
            if (pos > 0) {
                pos--;
                redraw();
            } else {
                putchar('\a');
                fflush(stdout);
            }
            continue;
        }

        if (uc == kill_char) {
            pos = 0;
            redraw();
            continue;
        }

        if (uc == 23) {
            if (pos > 0) {
                delete_word();
                redraw();
            } else {
                putchar('\a');
                fflush(stdout);
            }
            continue;
        }

        if (isprint(uc)) {
            if (pos < MAX_BUF - 1) {
                buffer[pos++] = (char)uc;
                buffer[pos] = '\0';
                redraw();
            } else {
                putchar('\a');
                fflush(stdout);
            }
            continue;
        }

        putchar('\a');
        fflush(stdout);
    }

    disable_raw_mode();
    return 0;
}
