#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    char *data;
    struct Node *next;
};

void append(struct Node **head, const char *str)
{
    struct Node *node = malloc(sizeof(struct Node));
    if (node == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    node->data = malloc(strlen(str) + 1);
    if (node->data == NULL) {
        perror("malloc");
        free(node);
        exit(EXIT_FAILURE);
    }

    strcpy(node->data, str);
    node->next = NULL;

    if (*head == NULL) {
        *head = node;
    } else {
        struct Node *current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = node;
    }
}

void print_list(struct Node *head)
{
    printf("%s\n", head->data);
    while (head->next != NULL) {
        head = head->next;
    }
    printf("%s\n", head->data);
}

void free_list(struct Node *head)
{
    while (head != NULL) {
        struct Node *next = head->next;
        free(head->data);
        free(head);
        head = next;
    }
}

int main(void)
{
    struct Node *head = NULL;
    char buffer[1024];

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        if (buffer[0] == '.') {
            break;
        }

        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }

        append(&head, buffer);
    }

    print_list(head);
    free_list(head);
    return 0;
}