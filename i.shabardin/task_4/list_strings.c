#define _XOPEN_SOURCE 700

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node {
    char *text;
    struct node *next;
};

static char *read_line(int *failed)
{
    char chunk[256];
    char *line;
    size_t capacity = sizeof(chunk);
    size_t length = 0;

    *failed = 0;
    line = malloc(capacity);
    if (line == NULL) {
        perror("malloc");
        *failed = 1;
        return NULL;
    }

    for (;;) {
        size_t count;

        if (fgets(chunk, sizeof(chunk), stdin) == NULL) {
            if (ferror(stdin)) {
                perror("fgets");
                *failed = 1;
            }
            if (length == 0 || *failed) {
                free(line);
                return NULL;
            }
            break;
        }

        count = strlen(chunk);
        if (count > SIZE_MAX - length - 1) {
            fputs("Input line is too long\n", stderr);
            free(line);
            *failed = 1;
            return NULL;
        }
        while (length + count + 1 > capacity) {
            char *larger;
            size_t next = capacity * 2;

            if (next <= capacity) {
                fputs("Input line is too long\n", stderr);
                free(line);
                *failed = 1;
                return NULL;
            }
            larger = realloc(line, next);
            if (larger == NULL) {
                perror("realloc");
                free(line);
                *failed = 1;
                return NULL;
            }
            line = larger;
            capacity = next;
        }
        memcpy(line + length, chunk, count);
        length += count;
        if (count != 0 && chunk[count - 1] == '\n') {
            break;
        }
    }
    line[length] = '\0';
    return line;
}

int main(void)
{
    struct node *head = NULL;
    struct node *tail = NULL;
    struct node *current;
    int result = EXIT_SUCCESS;

    for (;;) {
        char *line;
        int failed;

        line = read_line(&failed);
        if (line == NULL) {
            if (failed) {
                result = EXIT_FAILURE;
            }
            break;
        }
        if (line[0] == '.') {
            free(line);
            break;
        }

        current = malloc(sizeof(*current));
        if (current == NULL) {
            perror("malloc");
            free(line);
            result = EXIT_FAILURE;
            break;
        }
        current->text = line;
        current->next = NULL;
        if (tail == NULL) {
            head = current;
        } else {
            tail->next = current;
        }
        tail = current;
    }

    for (current = head; current != NULL; current = current->next) {
        size_t length = strlen(current->text);
        fputs(current->text, stdout);
        if (length == 0 || current->text[length - 1] != '\n') {
            putchar('\n');
        }
    }

    while (head != NULL) {
        current = head;
        head = head->next;
        free(current->text);
        free(current);
    }
    return result;
}
