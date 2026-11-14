#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// узел списка: строка и указатель на следующий
typedef struct Node {
    char *data;          // копия строки
    struct Node *next;   // следующий узел
} Node;

// добавить строку в конец списка
static void append_node(Node **head, const char *text)
{
    // выделяю память под узел
    Node *new_node = malloc(sizeof(*new_node));
    Node *current;

    // проверка выделения
    if (new_node == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // выделяю память под строку, +1 для '\0'
    new_node->data = malloc(strlen(text) + 1);
    if (new_node->data == NULL) {
        perror("malloc");
        free(new_node);
        exit(EXIT_FAILURE);
    }

    // копирую строку и обнуляю next
    strcpy(new_node->data, text);
    new_node->next = NULL;

    // если список пуст, новый узел становится первым
    if (*head == NULL) {
        *head = new_node;
        return;
    }

    // иначе иду до последнего узла
    current = *head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = new_node;
}

static void print_list(const Node *head)
{
    if (head == NULL) {
        return;                       // дошли до конца списка
    }

    print_list(head->next);           // сначала печатает следующий узел
    printf("%s\n", head->data);       // потом себя — поэтому порядок обратный
}

// печатаю все строки из списка
/*static void print_list(const Node *head)
{
    const Node *current = head;

    while (current != NULL) {
        printf("%s\n", current->data);
        current = current->next;
    }
}*/

// печатаю только первую и последнюю строку
/*static void print_first_last(const Node *head)
{
    const Node *current;

    if (head == NULL) {
        return;                  // пустой список — печатать нечего
    }

    printf("%s\n", head->data);  // первая строка — это голова

    current = head;
    while (current->next != NULL) {
        current = current->next; // дохожу до последнего
    }

    if (current != head) {       // если строка всего одна — не дублирую
        printf("%s\n", current->data);
    }
}
*/

// освобождаю память
static void free_list(Node *head)
{
    Node *next;

    while (head != NULL) {
        next = head->next; // запоминаю следующий узел
        free(head->data);  // сначала строку
        free(head);        // потом сам узел
        head = next;
    }
}

int main(void)
{
    Node *head = NULL;   // голова списка
    char buffer[1024];   // буфер для ввода

    printf("Enter strings (line starting with '.' ends input):\n");

    // читаю строки, пока есть ввод
    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        size_t length = strlen(buffer);

        // убираю перевод строки в конце
        while (length > 0 && (buffer[length - 1] == '\n' || buffer[length - 1] == '\r')) {
            buffer[--length] = '\0';
        }

        // точка в начале — конец ввода
        if (buffer[0] == '.') {
            break;
        }

        append_node(&head, buffer);
    }
    
    print_list(head); // вывожу результат
    free_list(head);  // чищу память
    return 0;
}