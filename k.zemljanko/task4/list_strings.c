#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    char *data;
    struct Node *next;
};

void append(struct Node **head, const char *str) {
    struct Node *new_node = malloc(sizeof(struct Node));

    if (new_node == NULL) {
        exit(EXIT_FAILURE);
    }

    new_node->data = malloc(strlen(str) + 1);

    if (new_node->data == NULL) {
        free(new_node);
        exit(EXIT_FAILURE);
    }

    strcpy(new_node->data, str);
    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
    } else {
        struct Node *current = *head;

        while (current->next != NULL) {
            current = current->next;
        }

        current->next = new_node;
    }
}

void free_list(struct Node *head) {
    struct Node *current = head;

    while (current != NULL) {
        struct Node *next = current->next;

        free(current->data);
        free(current);

        current = next;
    }
}

int main(void) {
    struct Node *head = NULL;
    char buffer[1024];

    while (1) {
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }

        size_t len = strlen(buffer);

        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }

        if (buffer[0] == '.') {
            break;
        }

        if (buffer[0] != '\0') {
            append(&head, buffer);
        }
    }

    struct Node *current = head;

    while (current != NULL) {
        printf("%s\n", current->data);
        current = current->next;
    }

    free_list(head);

    return 0;
}
