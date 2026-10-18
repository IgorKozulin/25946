#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    char *data;
    struct Node *next;
};

void add(struct Node **head, char *str) {
    struct Node *new_node;

    new_node = malloc(sizeof(struct Node));

    if (new_node == NULL) {
        printf("Memory allocation error\n");
        return;
    }

    new_node->data = malloc(strlen(str) + 1);

    if (new_node->data == NULL) {
        printf("Memory allocation error\n");
        free(new_node);
        return;
    }

    strcpy(new_node->data, str);

    new_node->next = *head;
    *head = new_node;
}

void free_list(struct Node *head) {
    struct Node *next;

    while (head != NULL) {
        next = head->next;

        free(head->data);
        free(head);

        head = next;
    }
}

int main() {
    struct Node *head = NULL;
    struct Node *current;

    char buffer[1024];

    while (1) {

        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }

        if (buffer[0] == '.') {
            break;
        }

        if (strlen(buffer) > 0 &&
            buffer[strlen(buffer) - 1] == '\n') {

            buffer[strlen(buffer) - 1] = '\0';
        }

        if (strlen(buffer) > 0) {
            add(&head, buffer);
        }
    }

    current = head;

    while (current != NULL) {
        printf("%s\n", current->data);
        current = current->next;
    }

    free_list(head);

    return 0;
}
