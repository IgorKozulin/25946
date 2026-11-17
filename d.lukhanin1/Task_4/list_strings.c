#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    char *text;
    struct Node *next;
};

static int read_line(char **text) {
    char buffer[1024];
    size_t length = 0;
    char *line = malloc(1);
    if (line == NULL) {
        perror("malloc");
        return -1;
    }
    line[0] = '\0';

    while (fgets(buffer, sizeof buffer, stdin) != NULL) {
        if (length == 0 && buffer[0] == '.') {
            free(line);
            return 0;
        }
        size_t part = strlen(buffer);
        if (part > SIZE_MAX - length - 1) {
            fprintf(stderr, "Line is too long\n");
            free(line);
            return -1;
        }
        char *grown = realloc(line, length + part + 1);
        if (grown == NULL) {
            perror("realloc");
            free(line);
            return -1;
        }
        line = grown;
        memcpy(line + length, buffer, part + 1);
        length += part;
        if (length != 0 && line[length - 1] == '\n') {
            *text = line;
            return 1;
        }
    }
    if (ferror(stdin)) {
        perror("fgets");
        free(line);
        return -1;
    }
    if (length != 0) {
        *text = line;
        return 1;
    }
    free(line);
    return 0;
}

static void free_list(struct Node *head) {
    while (head != NULL) {
        struct Node *next = head->next;
        free(head->text);
        free(head);
        head = next;
    }
}

int main(void) {
    struct Node *head = NULL;
    struct Node *tail = NULL;
    char *text;
    int status;
    int result = EXIT_SUCCESS;

    fprintf(stderr, "Enter lines; a leading dot finishes input.\n");
    while ((status = read_line(&text)) == 1) {
        struct Node *node = malloc(sizeof *node);
        if (node == NULL) {
            perror("malloc");
            free(text);
            result = EXIT_FAILURE;
            break;
        }
        node->text = text;
        node->next = NULL;
        if (tail == NULL) {
            head = node;
        } else {
            tail->next = node;
        }
        tail = node;
    }
    if (status == -1) {
        result = EXIT_FAILURE;
    }
    if (result == EXIT_SUCCESS) {
        if (head != NULL && fputs(head->text, stdout) == EOF) {
            perror("fputs");
            result = EXIT_FAILURE;
        }
        if (result == EXIT_SUCCESS && tail != head && fputs(tail->text, stdout) == EOF) {
            perror("fputs");
            result = EXIT_FAILURE;
        }
        if (fflush(stdout) == EOF) {
            perror("fflush");
            result = EXIT_FAILURE;
        }
    }
    free_list(head);
    return result;
}
