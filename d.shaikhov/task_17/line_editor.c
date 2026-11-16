
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>

#define MAX_TEXT 4096
#define LINE_WIDTH 40

struct termios original_term;

char buffer[MAX_TEXT];
char undo_buffer[MAX_TEXT];

int pos = 0;
int undo_pos = -1;
int drawn_rows = 0;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_term);
}

int enable_raw_mode() {
    struct termios term;

    if (tcgetattr(STDIN_FILENO, &original_term) == -1) {
        printf("tcgetattr error\n");
        return 0;
    }

    term = original_term;
    term.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    term.c_iflag &= ~IXON;
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) == -1) {
        printf("tcsetattr error\n");
        return 0;
    }

    if (atexit(disable_raw_mode) != 0) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_term);
        printf("atexit error\n");
        return 0;
    }

    return 1;
}

void save_undo() {
    memcpy(undo_buffer, buffer, pos);
    undo_pos = pos;
}

void redraw() {
    int col = 0;
    int rows = 1;

    if (drawn_rows > 0) {
        printf("\r\033[K");

        for (int k = 1; k < drawn_rows; k++) {
            printf("\033[1A\r\033[K");
        }
    }

    printf("> ");

    for (int i = 0; i < pos;) {
        if (buffer[i] == ' ') {
            if (col == LINE_WIDTH) {
                putchar('\n');
                rows++;
                col = 0;
            }

            putchar(' ');
            col++;
            i++;
        } else {
            int word_end = i;

            while (word_end < pos && buffer[word_end] != ' ') {
                word_end++;
            }

            int word_length = word_end - i;

            if (col > 0 && col + word_length > LINE_WIDTH) {
                putchar('\n');
                rows++;
                col = 0;
            }

            while (i < word_end) {
                if (col == LINE_WIDTH) {
                    putchar('\n');
                    rows++;
                    col = 0;
                }

                putchar(buffer[i]);
                col++;
                i++;
            }
        }
    }

    drawn_rows = rows;
    fflush(stdout);
}

int main() {
    unsigned char c;

    if (!enable_raw_mode()) {
        return 1;
    }

    redraw();

    while (read(STDIN_FILENO, &c, 1) == 1) {

        if (c == 4) {
            if (pos == 0) {
                printf("\r\033[KExiting...\n");
                break;
            }

            putchar('\a');
            fflush(stdout);
        }

        else if (c == original_term.c_cc[VERASE]) {
            if (pos > 0) {
                save_undo();
                pos--;
                redraw();
            }
        }

        else if (c == original_term.c_cc[VKILL]) {
            if (pos > 0) {
                save_undo();
                pos = 0;
                redraw();
            }
        }

        else if (c == 23) {
            if (pos > 0) {
                save_undo();

                while (pos > 0 && buffer[pos - 1] == ' ') pos--;
                while (pos > 0 && buffer[pos - 1] != ' ') pos--;
                while (pos > 0 && buffer[pos - 1] == ' ') pos--;

                redraw();
            }
        }

        else if (c == 26) {
            if (undo_pos != -1) {
                memcpy(buffer, undo_buffer, undo_pos);
                pos = undo_pos;
                undo_pos = -1;
                redraw();
            }
        }

        else if (c == '\n' || c == '\r') {
            putchar('\n');
            pos = 0;
            undo_pos = -1;
            drawn_rows = 0;
            redraw();
        }

        else if (isprint(c)) {
            if (pos < MAX_TEXT - 1) {
                undo_pos = -1;
                buffer[pos] = c;
                pos++;
                redraw();
            } else {
                putchar('\a');
                fflush(stdout);
            }
        }

        else {
            putchar('\a');
            fflush(stdout);
        }
    }

    return 0;
}

