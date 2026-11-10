#if defined(__sun) && !defined(__EXTENSIONS__)
/* Открываем объявления размера окна терминала в заголовках Solaris */
#define __EXTENSIONS__ 1
#endif
#define _XOPEN_SOURCE 700
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#ifdef __sun
#include <stropts.h>
#endif
#include <termios.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

#define WIDTH 40
#define CAPACITY 4096

static struct termios saved;
static int terminal_changed;
static volatile sig_atomic_t stopped;

typedef struct Deletion {
    struct Deletion *previous;
    size_t position;
    size_t length;
    wchar_t text[];
} Deletion;

static void clear_history(Deletion **history)
{
    while (*history != NULL) {
        Deletion *previous = (*history)->previous;
        free(*history);
        *history = previous;
    }
}

static int remember_deletion(Deletion **history, const wchar_t *text,
                             size_t begin, size_t end)
{
    Deletion *item;
    if (begin == end) return 0;
    item = malloc(sizeof(*item) + (end - begin) * sizeof(*text));
    if (item == NULL) return -1;
    item->previous = *history;
    item->position = begin;
    item->length = end - begin;
    memcpy(item->text, text + begin, item->length * sizeof(*text));
    *history = item;
    return 0;
}

static void restore_terminal(void)
{
    if (terminal_changed) {
        while (tcsetattr(STDIN_FILENO, TCSANOW, &saved) == -1) {
            if (errno != EINTR) break;
        }
        terminal_changed = 0;
    }
}

static void stop_handler(int number)
{
    stopped = number;
}

static void emit(wchar_t character)
{
    char bytes[MB_LEN_MAX];
    mbstate_t state = {0};
    size_t count = wcrtomb(bytes, character, &state);
    if (count != (size_t)-1) fwrite(bytes, 1, count, stdout);
}

/* Один алгоритм считает строки экрана и рисует текст с переносом слов */
static size_t layout(const wchar_t *text, size_t length, int draw)
{
    size_t row = 0, column = 0, i = 0;
    while (i < length) {
        size_t end = i + 1, word_width = 0;
        if (text[i] != L' ') {
            while (end < length && text[end] != L' ') ++end;
            for (size_t j = i; j < end; ++j) word_width += (size_t)wcwidth(text[j]);
            if (column != 0 && word_width > WIDTH - column) {
                if (draw) fputs("\r\n", stdout);
                ++row;
                column = 0;
            }
        }
        for (; i < end; ++i) {
            size_t width = (size_t)wcwidth(text[i]);
            if (column + width > WIDTH) {
                if (draw) fputs("\r\n", stdout);
                ++row;
                column = 0;
            }
            if (draw) emit(text[i]);
            column += width;
        }
    }
    return row;
}

static size_t redraw(const wchar_t *text, size_t length, size_t old_row)
{
    /* Возвращаемся к началу редактируемого блока и очищаем прежние строки */
    putchar('\r');
    if (old_row != 0) printf("\033[%zuA", old_row);
    fputs("\033[J", stdout);
    old_row = layout(text, length, 1);
    fflush(stdout);
    return old_row;
}

int main(void)
{
    struct termios mode;
    struct winsize screen = {0};
    struct sigaction action;
    wchar_t text[CAPACITY];
    Deletion *history = NULL;
    size_t length = 0, row = 0, max_rows;
    mbstate_t decoder = {0};
    int status = EXIT_SUCCESS;
    const int signals[] = {
        SIGINT, SIGTERM, SIGHUP, SIGQUIT, SIGTSTP
#ifdef SIGWINCH
        , SIGWINCH
#endif
    };

    if (setlocale(LC_CTYPE, "") == NULL) {
        const char *fallbacks[] = { "C.UTF-8", "en_US.UTF-8", "UTF-8", "C" };
        const char *selected = NULL;
        /* Если локаль сеанса недоступна, пробуем UTF-8 и затем стандартную C */
        for (size_t i = 0; i < sizeof(fallbacks) / sizeof(fallbacks[0]); ++i) {
            selected = setlocale(LC_CTYPE, fallbacks[i]);
            if (selected != NULL) break;
        }
        if (selected == NULL) {
            fputs("Cannot initialize locale\n", stderr);
            return EXIT_FAILURE;
        }
        fprintf(stderr, "Unsupported locale settings, using %s\n", selected);
        if (MB_CUR_MAX == 1) {
            fputs("Use ASCII text in this locale\n", stderr);
        }
    }
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        fputs("Run this program in an interactive terminal\n", stderr);
        return EXIT_FAILURE;
    }
    if (tcgetattr(STDIN_FILENO, &saved) == -1) { perror("tcgetattr"); return EXIT_FAILURE; }
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &screen) == -1 || screen.ws_row == 0) screen.ws_row = 24;
    if (screen.ws_col != 0 && screen.ws_col <= WIDTH) {
        fputs("Make the terminal wider than 40 columns\n", stderr);
        return EXIT_FAILURE;
    }
    max_rows = screen.ws_row > 2 ? screen.ws_row - 2 : 1;
    if (atexit(restore_terminal) != 0) return EXIT_FAILURE;
    memset(&action, 0, sizeof(action));
    action.sa_handler = stop_handler;
    sigemptyset(&action.sa_mask);
    for (size_t i = 0; i < sizeof(signals) / sizeof(signals[0]); ++i) {
        if (sigaction(signals[i], &action, NULL) == -1) { perror("sigaction"); return EXIT_FAILURE; }
    }

    puts("Line editor: ERASE, KILL, Ctrl-W; Ctrl-Z undoes deletion; Enter starts a new line; Ctrl-D exits on an empty line");
    fflush(stdout);

    mode = saved;
    /* Управляющие клавиши получаем как байты и обрабатываем самостоятельно */
    mode.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
    mode.c_iflag &= ~(IXON | IXOFF | ISTRIP | INLCR | IGNCR | ICRNL);
    mode.c_cc[VMIN] = 1;
    mode.c_cc[VTIME] = 0;
    terminal_changed = 1;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &mode) == -1) { perror("tcsetattr"); return EXIT_FAILURE; }

    while (!stopped) {
        unsigned char byte;
        struct pollfd input = { STDIN_FILENO, POLLIN, 0 };
        int ready = poll(&input, 1, 100);
        ssize_t count;
        wchar_t character;
        size_t decoded;
        if (ready == -1) {
            if (errno == EINTR) continue;
            perror("poll"); status = EXIT_FAILURE; break;
        }
        if (stopped || ready == 0) continue;
        count = read(STDIN_FILENO, &byte, 1);
        if (count == 0) break;
        if (count == -1) {
            if (errno == EINTR) continue;
            perror("read"); status = EXIT_FAILURE; break;
        }

        if (byte < 32 || byte == 127 ||
            (saved.c_cc[VERASE] != (cc_t)_POSIX_VDISABLE && byte == saved.c_cc[VERASE]) ||
            (saved.c_cc[VKILL] != (cc_t)_POSIX_VDISABLE && byte == saved.c_cc[VKILL])) {
            memset(&decoder, 0, sizeof(decoder));
            if (byte == 26) {
                /* Возвращаем удаленный фрагмент на его место, сохраняя последующий ввод */
                if (history == NULL || history->length > CAPACITY - length) {
                    putchar('\a');
                } else {
                    wchar_t restored[CAPACITY];
                    size_t at = history->position;
                    size_t new_length = length + history->length;
                    memcpy(restored, text, at * sizeof(*text));
                    memcpy(restored + at, history->text, history->length * sizeof(*text));
                    memcpy(restored + at + history->length, text + at, (length - at) * sizeof(*text));
                    if (layout(restored, new_length, 0) >= max_rows) {
                        putchar('\a');
                    } else {
                        Deletion *previous = history->previous;
                        memcpy(text, restored, new_length * sizeof(*text));
                        length = new_length;
                        free(history);
                        history = previous;
                    }
                }
            } else if ((saved.c_cc[VERASE] != (cc_t)_POSIX_VDISABLE && byte == saved.c_cc[VERASE]) ||
                       (saved.c_cc[VKILL] != (cc_t)_POSIX_VDISABLE && byte == saved.c_cc[VKILL]) || byte == 23) {
                size_t new_length = length;
                if (byte == saved.c_cc[VERASE]) {
                    if (new_length != 0) --new_length;
                } else if (byte == saved.c_cc[VKILL]) {
                    new_length = 0;
                } else {
                    while (new_length != 0 && text[new_length - 1] == L' ') --new_length;
                    while (new_length != 0 && text[new_length - 1] != L' ') --new_length;
                }
                /* Сохраняем все удаление одним действием, прежде чем менять длину */
                if (remember_deletion(&history, text, new_length, length) == -1) {
                    putchar('\a');
                } else {
                    length = new_length;
                }
            } else if (byte == 4) {
                if (length == 0) break;
                putchar('\a');
            } else if (byte == '\n' || byte == '\r') {
                fputs("\r\n", stdout);
                length = 0;
                row = 0;
                clear_history(&history);
            } else {
                putchar('\a');
            }
            row = redraw(text, length, row);
            continue;
        }

        /* UTF-8 собираем в символы, чтобы Backspace не удалял половину буквы */
        decoded = mbrtowc(&character, (const char *)&byte, 1, &decoder);
        if (decoded == (size_t)-2) continue;
        if (decoded == (size_t)-1) {
            memset(&decoder, 0, sizeof(decoder));
            putchar('\a'); fflush(stdout); continue;
        }
        if (!iswprint(character) || wcwidth(character) < 0 || length == CAPACITY) {
            putchar('\a'); fflush(stdout); continue;
        }
        text[length++] = character;
        /* Не допускаем прокрутку начала текущей строки за верхнюю границу экрана */
        if (layout(text, length, 0) >= max_rows) {
            --length;
            putchar('\a');
        }
        row = redraw(text, length, row);
        if (ferror(stdout)) { status = EXIT_FAILURE; break; }
    }
    clear_history(&history);
    restore_terminal();
    puts("\nExiting...");
    return stopped ? 128 + stopped : status;
}
