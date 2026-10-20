#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024

struct Node {
    char *data;
    struct Node *next;
};

void append(struct Node **head, const char *str)
{
    struct Node *new_node = malloc(sizeof(struct Node));
    if (new_node == NULL) {
        fprintf(stderr, "Error: failed to allocate memory for node\n");
        exit(EXIT_FAILURE);
    }

    size_t len = strlen(str);
    new_node->data = malloc(len + 1);
    if (new_node->data == NULL) {
        fprintf(stderr, "Error: failed to allocate memory for string\n");
        free(new_node);
        exit(EXIT_FAILURE);
    }
    strcpy(new_node->data, str);

    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
    } else {
        struct Node *cur = *head;
        while (cur->next != NULL) {
            cur = cur->next;
        }
        cur->next = new_node;
    }
}

void print_list(struct Node *head)
{
    struct Node *cur = head;
    while (cur != NULL) {
        printf("%s\n", cur->data);
        cur = cur->next;
    }
}

void free_list(struct Node *head)
{
    struct Node *cur = head;
    while (cur != NULL) {
        struct Node *next = cur->next;
        free(cur->data);
        free(cur);
        cur = next;
    }
}

int main(void)
{
    struct Node *head = NULL;
    char buffer[BUFFER_SIZE];

    printf("Enter strings (a dot at the beginning of a line ends input):\n");

    while (1) {
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }

        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }

        if (len > 0 && buffer[0] == '.') {
            break;
        }

        append(&head, buffer);
    }

    printf("\nEntered strings:\n");
    print_list(head);

    free_list(head);

    return 0;
}
