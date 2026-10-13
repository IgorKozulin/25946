#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    char *data;
    struct Node *next;
};

void append(struct Node **head, const char *str) {
    struct Node *new_node = (struct Node *)malloc(sizeof(struct Node));
    if (new_node == NULL) {
        fprintf(stderr, "Ошибка выделения памяти для узла\n");
        exit(1);
    }
    size_t len = strlen(str);
    new_node->data = (char *)malloc(len + 1);
    if (new_node->data == NULL) {
        fprintf(stderr, "Ошибка выделения памяти для строки\n");
        free(new_node);
        exit(1);
    }
    strcpy(new_node->data, str); 
    new_node->next = NULL;
    if (*head == NULL) {
        *head = new_node;
        return;
    }
    struct Node *current = *head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = new_node;
}

void free_list(struct Node *head) {
    struct Node *current = head;
    struct Node *next;
    
    while (current != NULL) {
        next = current->next;
        free(current->data);
        free(current);
        current = next;
    }
}
void print_list(struct Node *head) {
    struct Node *current = head;
    while (current != NULL) {
        printf("%s\n", current->data);
        current = current->next;
    }
}

int main() {
    struct Node *head = NULL;
    char buffer[1024];
    printf("Введите строки (точка '.' в начале строки завершает ввод):\n");
    
    while (1) {
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
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
        if (len > 0) {
            append(&head, buffer);
        }
    }
    printf("\nСписок строк:\n");
    print_list(head);
    free_list(head);
    return 0;
}
