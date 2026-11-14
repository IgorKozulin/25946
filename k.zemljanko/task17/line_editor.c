#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <string.h>
#include <poll.h>

#define MAX 4096
#define WIDTH 40

struct termios old;

char text[MAX];
int n = 0, cursor = 0, move_mode = 0;
int last_row = 0;

int row[MAX], col[MAX];

void restore(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &old);
}

void redraw(void) {
    int r = 0, c = 2;

    printf("\r");

    if (last_row > 0)
        printf("\033[%dA", last_row);

    printf("\r\033[J%s", move_mode ? "M>" : "> ");

    row[0] = r;
    col[0] = c;

    for (int i = 0; i < n;) {
        if (text[i] == ' ') {
            if (c == WIDTH) {
                putchar('\n');
                r++;
                c = 0;
                row[i] = r;
                col[i] = c;
            }

            putchar(' ');
            c++;

            row[++i] = r;
            col[i] = c;

        } else {
            int end = i;

            while (end < n && text[end] != ' ')
                end++;

            if (c > 0 && c + end - i > WIDTH) {
                putchar('\n');
                r++;
                c = 0;
                row[i] = r;
                col[i] = c;
            }

            while (i < end) {
                if (c == WIDTH) {
                    putchar('\n');
                    r++;
                    c = 0;
                    row[i] = r;
                    col[i] = c;
                }

                putchar(text[i]);
                c++;

                row[++i] = r;
                col[i] = c;
            }
        }
    }

    if (r > row[cursor])
        printf("\033[%dA", r - row[cursor]);
    else if (r < row[cursor])
        printf("\033[%dB", row[cursor] - r);

    printf("\r");

    if (col[cursor] > 0)
        printf("\033[%dC", col[cursor]);

    last_row = row[cursor];

    fflush(stdout);
}

int next_byte(void) {
    struct pollfd p;
    unsigned char c;

    p.fd = STDIN_FILENO;
    p.events = POLLIN;
    p.revents = 0;

    if (poll(&p, 1, 100) <= 0)
        return -1;

    if (read(STDIN_FILENO, &c, 1) != 1)
        return -1;

    return c;
}

int get_key(void) {
    unsigned char c;

    if (read(STDIN_FILENO, &c, 1) != 1)
        return -1;

    if (c != 27)
        return c;

    int a = next_byte();

    if (a != '[' && a != 'O')
        return 27;

    int b = next_byte();

    if (b == 'A') return 1001;
    if (b == 'B') return 1002;
    if (b == 'C') return 1003;
    if (b == 'D') return 1004;

    return 27;
}

void vertical(int direction) {
    int target = row[cursor] + direction;
    int best = -1;
    int distance = 10000;

    for (int i = 0; i <= n; i++) {
        if (row[i] != target)
            continue;

        int d = abs(col[i] - col[cursor]);

        if (d < distance) {
            distance = d;
            best = i;
        }
    }

    if (best >= 0)
        cursor = best;
    else
        putchar('\a');
}

int main(void) {
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, &old) == -1) {
        perror("tcgetattr");
        return 1;
    }

    raw = old;

    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_iflag &= ~IXON;
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        return 1;
    }

    atexit(restore);

    redraw();

    while (1) {
        int k = get_key();

        if (k < 0)
            break;

        if (k == 16) {
            move_mode = !move_mode;

        } else if (k >= 1001 && k <= 1004) {
            if (!move_mode) {
                putchar('\a');
                continue;
            }

            if (k == 1004 && cursor > 0)
                cursor--;
            else if (k == 1003 && cursor < n)
                cursor++;
            else if (k == 1001)
                vertical(-1);
            else if (k == 1002)
                vertical(1);
            else
                putchar('\a');

        } else if (k == 4 && cursor == 0) {
            printf("\nExiting...\n");
            break;

        } else if (move_mode) {
            putchar('\a');
            continue;

        } else if (k == old.c_cc[VERASE] || k == 8 || k == 127) {
            if (cursor > 0) {
                memmove(text + cursor - 1,
                        text + cursor,
                        n - cursor);

                cursor--;
                n--;
            }

        } else if (k == old.c_cc[VKILL]) {
            cursor = n = 0;

        } else if (k == 23) {
            int start = cursor;

            while (start > 0 && text[start - 1] == ' ')
                start--;

            while (start > 0 && text[start - 1] != ' ')
                start--;

            while (start > 0 && text[start - 1] == ' ')
                start--;

            memmove(text + start, text + cursor, n - cursor);

            n -= cursor - start;
            cursor = start;

        } else if (k == '\r' || k == '\n') {
            cursor = n;
            redraw();

            putchar('\n');

            cursor = n = 0;
            last_row = 0;

            printf("> ");
            fflush(stdout);
            continue;

        } else if (k < 256 && isprint((unsigned char)k)
                   && n < MAX - 1) {

            memmove(text + cursor + 1,
                    text + cursor,
                    n - cursor);

            text[cursor++] = (char)k;
            n++;

        } else {
            putchar('\a');
            fflush(stdout);
            continue;
        }

        redraw();
    }

    return 0;
}
