#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#define WIDTH 40

static volatile sig_atomic_t interrupted;
static sigset_t managed_signals;

static void on_signal(int number)
{
    interrupted = number;
}

static int output(const char *text, size_t size)
{
    while (size > 0 && !interrupted) {
        ssize_t count = write(STDOUT_FILENO, text, size);
        if (count < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (count == 0) {
            errno = EIO;
            return -1;
        }
        text += count;
        size -= (size_t)count;
    }
    return interrupted ? -1 : 0;
}

static int erase(size_t count)
{
    while (count-- > 0) {
        if (output("\b \b", 3) < 0)
            return -1;
    }
    return 0;
}

static ssize_t input(unsigned char *c)
{
    sigset_t previous;
    fd_set descriptors;
    ssize_t count = -1;
    int saved_errno;

    if (sigprocmask(SIG_BLOCK, &managed_signals, &previous) < 0)
        return -1;
    if (interrupted) {
        errno = EINTR;
    } else {
        FD_ZERO(&descriptors);
        FD_SET(STDIN_FILENO, &descriptors);
        int ready = pselect(STDIN_FILENO + 1, &descriptors, NULL, NULL,
                            NULL, &previous);
        if (ready > 0)
            count = read(STDIN_FILENO, c, 1);
    }
    saved_errno = errno;
    if (sigprocmask(SIG_SETMASK, &previous, NULL) < 0)
        return -1;
    errno = saved_errno;
    return count;
}

static int edit(const struct termios *original)
{
    char line[WIDTH];
    size_t length = 0;

    while (!interrupted) {
        unsigned char c;
        ssize_t count = input(&c);
        if (count < 0) {
            if (errno == EINTR)
                continue;
            perror("read");
            return 1;
        }
        if (count == 0)
            return 0;
        if (c == 4 && length == 0)
            return 0;
        if (original->c_cc[VERASE] != (cc_t)_POSIX_VDISABLE &&
            c == original->c_cc[VERASE]) {
            if (length > 0) {
                if (erase(1) < 0)
                    goto write_error;
                --length;
            }
        } else if (original->c_cc[VKILL] != (cc_t)_POSIX_VDISABLE &&
                   c == original->c_cc[VKILL]) {
            if (erase(length) < 0)
                goto write_error;
            length = 0;
        } else if (c == 23) {
            size_t end = length;
            while (length > 0 && line[length - 1] == ' ')
                --length;
            while (length > 0 && line[length - 1] != ' ')
                --length;
            if (erase(end - length) < 0)
                goto write_error;
        } else if (c == '\n' || c == '\r') {
            if (output("\r\n", 2) < 0)
                goto write_error;
            length = 0;
        } else if (c >= 32 && c <= 126) {
            if (length == WIDTH) {
                size_t start = length;
                if (c != ' ') {
                    while (start > 0 && line[start - 1] != ' ')
                        --start;
                }
                if (start > 0 && start < length) {
                    size_t word_length = length - start;
                    if (erase(word_length) < 0 || output("\r\n", 2) < 0)
                        goto write_error;
                    memmove(line, line + start, word_length);
                    length = word_length;
                    if (output(line, length) < 0)
                        goto write_error;
                } else {
                    if (output("\r\n", 2) < 0)
                        goto write_error;
                    length = 0;
                }
            }
            line[length++] = (char)c;
            if (output((const char *)&c, 1) < 0)
                goto write_error;
        } else if (output("\a", 1) < 0) {
            goto write_error;
        }
    }
    return 0;

write_error:
    if (!interrupted)
        perror("write");
    return 1;
}

int main(void)
{
    struct termios original;
    struct termios settings;
    struct sigaction action;
    const int signals[] = {SIGINT, SIGTERM, SIGHUP, SIGQUIT, SIGPIPE, SIGTSTP};
    int result;

    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        fprintf(stderr, "Run line_editor in a terminal without redirection.\n");
        return 1;
    }
    if (tcgetattr(STDIN_FILENO, &original) < 0) {
        perror("tcgetattr");
        return 1;
    }
    memset(&action, 0, sizeof(action));
    action.sa_handler = on_signal;
    sigemptyset(&action.sa_mask);
    sigemptyset(&managed_signals);
    for (size_t i = 0; i < sizeof(signals) / sizeof(signals[0]); ++i) {
        sigaddset(&managed_signals, signals[i]);
        if (sigaction(signals[i], &action, NULL) < 0) {
            perror("sigaction");
            return 1;
        }
    }
    settings = original;
    settings.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
    settings.c_iflag &= ~(ICRNL | INLCR | IGNCR | IXON | IXOFF | ISTRIP);
    settings.c_oflag &= ~OPOST;
    settings.c_cc[VMIN] = 1;
    settings.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &settings) < 0) {
        perror("tcsetattr");
        return 1;
    }
    result = edit(&original);
    while (tcsetattr(STDIN_FILENO, TCSANOW, &original) < 0) {
        if (errno == EINTR)
            continue;
        perror("restore terminal");
        return 1;
    }
    return interrupted ? 128 + interrupted : result;
}
