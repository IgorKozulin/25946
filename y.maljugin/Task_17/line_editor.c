#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define WIDTH 40
#define MAX_INPUT 4096

static struct termios saved_terminal;
static int terminal_changed;
static size_t screen_rows = 1;

static void restore_terminal(void)
{
    if (terminal_changed)
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_terminal);
}

static void bell(void)
{
    putchar('\a');
    fflush(stdout);
}

static void new_screen_row(size_t *column, size_t *rows)
{
    putchar('\n');
    *column = 0;
    ++*rows;
}

/* Redraw the current logical line after every edit. */
static void redraw(const char *text, size_t length)
{
    size_t column = 0;
    size_t rows = 1;

    putchar('\r');
    if (screen_rows > 1)
        printf("\033[%luA", (unsigned long)(screen_rows - 1));
    fputs("\033[J", stdout);

    for (size_t i = 0; i < length;) {
        if (text[i] == ' ') {
            if (column == WIDTH)
                new_screen_row(&column, &rows);
            putchar(' ');
            ++column;
            ++i;
            continue;
        }

        size_t word_end = i;
        while (word_end < length && text[word_end] != ' ')
            ++word_end;
        size_t word_length = word_end - i;

        /* A word that still fits in 40 columns moves as one piece. */
        if (column != 0 && word_length <= WIDTH && column + word_length > WIDTH)
            new_screen_row(&column, &rows);
        while (i < word_end) {
            if (column == WIDTH)
                new_screen_row(&column, &rows);
            putchar(text[i++]);
            ++column;
        }
    }
    screen_rows = rows;
    fflush(stdout);
}

int main(void)
{
    if (!isatty(STDIN_FILENO) || tcgetattr(STDIN_FILENO, &saved_terminal) == -1) {
        fprintf(stderr, "Run this program in a terminal.\n");
        return EXIT_FAILURE;
    }
    if (atexit(restore_terminal) != 0) {
        perror("atexit");
        return EXIT_FAILURE;
    }

    struct termios raw = saved_terminal;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_iflag &= ~(IXON | ICRNL);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        return EXIT_FAILURE;
    }
    terminal_changed = 1;

    char text[MAX_INPUT];
    size_t length = 0;
    unsigned char ch;
    for (;;) {
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        if (n == -1 && errno == EINTR)
            continue;
        if (n == -1) {
            perror("read");
            return EXIT_FAILURE;
        }
        if (n == 0)
            break;

        if (ch == 4) {                 /* Ctrl-D */
            if (length == 0)
                break;
            bell();
        } else if (ch == saved_terminal.c_cc[VERASE] || ch == 8 || ch == 127) {
            if (length != 0)
                redraw(text, --length);
            else
                bell();
        } else if (ch == saved_terminal.c_cc[VKILL]) {
            length = 0;
            redraw(text, length);
        } else if (ch == 23) {        /* Ctrl-W */
            while (length != 0 && text[length - 1] == ' ')
                --length;
            while (length != 0 && text[length - 1] != ' ')
                --length;
            while (length != 0 && text[length - 1] == ' ')
                --length;
            redraw(text, length);
        } else if (ch == '\r' || ch == '\n') {
            putchar('\n');
            length = 0;
            screen_rows = 1;
        } else if (isprint(ch)) {
            if (length == sizeof(text))
                bell();
            else {
                text[length++] = (char)ch;
                redraw(text, length);
            }
        } else {
            bell();
        }
    }
    putchar('\n');
    puts("Exiting...");
    return EXIT_SUCCESS;
}
