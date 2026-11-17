#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

enum {
    LINE_WIDTH = 40,
    CTRL_D = 4,
    CTRL_W = 23
};

static struct termios saved_terminal;

static void restore_on_signal(int signo)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_terminal);
    _exit(128 + signo);
}

static int configure_terminal(void)
{
    struct termios terminal;
    struct sigaction action = {0};

    if (tcgetattr(STDIN_FILENO, &saved_terminal) == -1) {
        return -1;
    }

    action.sa_handler = restore_on_signal;
    sigfillset(&action.sa_mask);
    if (sigaction(SIGHUP, &action, NULL) == -1 ||
        sigaction(SIGINT, &action, NULL) == -1 ||
        sigaction(SIGQUIT, &action, NULL) == -1 ||
        sigaction(SIGTERM, &action, NULL) == -1 ||
        sigaction(SIGPIPE, &action, NULL) == -1) {
        return -1;
    }

    terminal = saved_terminal;
    terminal.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP |
                          INLCR | IGNCR | ICRNL | IXON | IXOFF | INPCK);
    terminal.c_oflag &= ~OPOST;
    terminal.c_cflag &= ~(CSIZE | PARENB);
    terminal.c_cflag |= CS8;
    terminal.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
    terminal.c_cc[VMIN] = 1;
    terminal.c_cc[VTIME] = 0;
    return tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal);
}

static void redraw_line(const char *line, size_t length)
{
    fputs("\r\033[K", stdout);
    fwrite(line, 1, length, stdout);
}

static size_t word_start(const char *line, size_t end)
{
    while (end > 0 && line[end - 1] != ' ') {
        --end;
    }
    return end;
}

static void append_character(char *line, size_t *length, unsigned char input)
{
    if (*length == LINE_WIDTH) {
        size_t start = word_start(line, *length);

        if (input != ' ' && start > 0 && start < *length) {
            /* Move the part of the word already visible on the full line. */
            redraw_line(line, start);
            fputs("\r\n", stdout);
            *length -= start;
            memmove(line, line + start, *length);
            fwrite(line, 1, *length, stdout);
        } else {
            /* A word longer than a full line has to be split. */
            fputs("\r\n", stdout);
            *length = 0;
        }
    }

    line[(*length)++] = (char)input;
    putchar(input);
}

static int edit_line(void)
{
    char line[LINE_WIDTH];
    size_t length = 0;
    unsigned char input;
    long disabled = fpathconf(STDIN_FILENO, _PC_VDISABLE);
    cc_t erase = saved_terminal.c_cc[VERASE];
    cc_t kill = saved_terminal.c_cc[VKILL];

    for (;;) {
        ssize_t count = read(STDIN_FILENO, &input, 1);

        if (count == 0) {
            return EXIT_SUCCESS;
        }
        if (count == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("read");
            return EXIT_FAILURE;
        }

        if (input == CTRL_D) {
            if (length == 0) {
                return EXIT_SUCCESS;
            }
        } else if (input == CTRL_W) {
            while (length > 0 && line[length - 1] == ' ') {
                --length;
            }
            length = word_start(line, length);
            redraw_line(line, length);
        } else if (erase != disabled && input == erase) {
            if (length > 0) {
                redraw_line(line, --length);
            }
        } else if (kill != disabled && input == kill) {
            length = 0;
            redraw_line(line, length);
        } else if (isprint(input)) {
            append_character(line, &length, input);
        } else {
            putchar('\a');
        }

        if (fflush(stdout) == EOF || ferror(stdout)) {
            perror("stdout");
            return EXIT_FAILURE;
        }
    }
}

int main(void)
{
    int status;
    int restored;

    if (configure_terminal() == -1) {
        perror("configure terminal");
        return EXIT_FAILURE;
    }

    status = edit_line();
    do {
        restored = tcsetattr(STDIN_FILENO, TCSANOW, &saved_terminal);
    } while (restored == -1 && errno == EINTR);
    if (restored == -1) {
        perror("restore terminal");
        return EXIT_FAILURE;
    }

    return status;
}
