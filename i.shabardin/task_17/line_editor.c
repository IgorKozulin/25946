#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define WIDTH 40
#define CAPACITY 4096

static struct termios saved_terminal;
static int terminal_changed = 0;

/* Возвращаем терминалу эхо и обычное редактирование при нормальном выходе. */
static void restore_terminal(void)
{
    if (terminal_changed) {
        tcsetattr(STDIN_FILENO, TCSANOW, &saved_terminal);
    }
}

/* Отключаем обработку клавиш ядром, чтобы получать каждый байт отдельно. */
static int enable_raw_mode(void)
{
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, &saved_terminal) == -1) {
        perror("tcgetattr (run in a terminal)");
        return -1;
    }
    if (atexit(restore_terminal) != 0) {
        fputs("atexit failed\n", stderr);
        return -1;
    }
    raw = saved_terminal;
    raw.c_lflag &= (tcflag_t)~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        return -1;
    }
    terminal_changed = 1;
    return 0;
}

/* Рисуем строку заново; слово переносится целиком, если его длина не больше 40. */
static size_t draw_text(const char *text, size_t length)
{
    size_t i = 0, column = 0, rows = 0;

    while (i < length) {
        size_t end, j;

        if (text[i] == ' ') {
            if (column == WIDTH) {
                putchar('\n');
                ++rows;
                column = 0;
            }
            putchar(' ');
            ++column;
            ++i;
            continue;
        }
        end = i;
        while (end < length && text[end] != ' ') {
            ++end;
        }
        if (end - i <= WIDTH && column != 0 && column + end - i > WIDTH) {
            putchar('\n');
            ++rows;
            column = 0;
        }
        for (j = i; j < end; ++j) {
            if (column == WIDTH) {
                putchar('\n');
                ++rows;
                column = 0;
            }
            putchar(text[j]);
            ++column;
        }
        i = end;
    }
    return rows;
}

/* Поднимаемся к началу старого вывода, очищаем его и рисуем новое состояние. */
static void redraw(const char *text, size_t length, size_t *old_rows)
{
    putchar('\r');
    if (*old_rows != 0) {
        printf("\033[%zuA", *old_rows);
    }
    fputs("\033[J", stdout);
    *old_rows = draw_text(text, length);
    fflush(stdout);
}

int main(void)
{
    char text[CAPACITY];
    size_t length = 0, rows = 0;
    unsigned char erase, kill;

    if (enable_raw_mode() == -1) {
        return EXIT_FAILURE;
    }
    erase = saved_terminal.c_cc[VERASE];
    kill = saved_terminal.c_cc[VKILL];
    puts("Type text; Enter starts a new line; Ctrl+D on an empty line exits.");
    fflush(stdout);

    for (;;) {
        unsigned char ch;
        ssize_t count = read(STDIN_FILENO, &ch, 1);

        if (count == -1 && errno == EINTR) {
            continue;
        }
        if (count == -1) {
            perror("read");
            return EXIT_FAILURE;
        }
        if (count == 0) {
            break;
        }
        if (ch == '\r' || ch == '\n') {
            putchar('\r');
            putchar('\n');
            length = rows = 0;
        } else if (ch == erase || ch == 8 || ch == 127) {
            if (length != 0) {
                --length;
                redraw(text, length, &rows);
            } else {
                putchar('\a');
            }
        } else if (ch == kill) {
            length = 0;
            redraw(text, length, &rows);
        } else if (ch == 23) { /* Ctrl+W: убрать пробелы и последнее слово. */
            while (length != 0 && text[length - 1] == ' ') {
                --length;
            }
            while (length != 0 && text[length - 1] != ' ') {
                --length;
            }
            redraw(text, length, &rows);
        } else if (ch == 4) { /* Ctrl+D действует только в начале логической строки. */
            if (length == 0) {
                putchar('\n');
                break;
            }
            putchar('\a');
        } else if (isprint(ch)) {
            if (length < sizeof(text)) {
                text[length++] = (char)ch;
                redraw(text, length, &rows);
            } else {
                putchar('\a');
            }
        } else {
            putchar('\a');
        }
        fflush(stdout);
    }
    return EXIT_SUCCESS;
}
