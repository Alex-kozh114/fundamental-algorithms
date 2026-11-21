#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#define START_CAPACITY 16

typedef enum
{
    STATUS_OK,
    STATUS_ERROR_OUT_OF_MEMORY,
    STATUS_ERROR_INVALID_ARGUMENT,
    STATUS_ERROR_FILE_OPEN,
    STATUS_ERROR_FILE_WRITE,
    STATUS_ERROR_FILE_FORMAT,
    STATUS_ERROR_END_OF_INPUT,
    STATUS_ERROR_EMPTY_TREE,
    STATUS_ERROR_OVERFLOW
} status_code_t;

typedef struct Node
{
    char *word;
    int count;
    struct Node *left;
    struct Node *right;
} Node;

static void tree_free(Node *node)
{
    if (node == NULL)
        return;
    tree_free(node->left);
    tree_free(node->right);
    free(node->word);
    free(node);
}

static status_code_t node_create(const char *word, const int count, Node **out)
{
    if (word == NULL || out == NULL || count <= 0)
        return STATUS_ERROR_INVALID_ARGUMENT;

    Node *node = (Node *)malloc(sizeof(Node));
    char *copy = (char *)malloc(strlen(word) + 1);
    if (node == NULL || copy == NULL)
    {
        free(node);
        free(copy);
        return STATUS_ERROR_OUT_OF_MEMORY;
    }
    strcpy(copy, word);
    node->word = copy;
    node->count = count;
    node->left = NULL;
    node->right = NULL;
    *out = node;
    return STATUS_OK;
}

static status_code_t append_char(char **buf, int *length, int *capacity, const char c)
{
    if (buf == NULL || *buf == NULL || length == NULL || capacity == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;

    if (*length + 1 >= *capacity)
    {
        if (*capacity > INT_MAX / 2)
            return STATUS_ERROR_OVERFLOW;
        const int new_capacity = *capacity * 2;
        char *new_buf = (char *)realloc(*buf, new_capacity);
        if (new_buf == NULL)
            return STATUS_ERROR_OUT_OF_MEMORY;
        *buf = new_buf;
        *capacity = new_capacity;
    }
    (*buf)[(*length)++] = c;
    (*buf)[*length] = '\0';
    return STATUS_OK;
}

static status_code_t read_dynamic_line(FILE *file, char **out)
{
    if (file == NULL || out == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;

    int length = 0;
    int capacity = START_CAPACITY;
    char *line = (char *)malloc(capacity);
    if (line == NULL)
        return STATUS_ERROR_OUT_OF_MEMORY;
    line[0] = '\0';

    int c;
    status_code_t sc = STATUS_OK;
    while (sc == STATUS_OK && (c = fgetc(file)) != EOF && c != '\n')
    {
        if (c != '\r')
            sc = append_char(&line, &length, &capacity, (char)c);
    }

    if (sc == STATUS_OK && c == EOF && length == 0)
        sc = STATUS_ERROR_END_OF_INPUT;
    if (sc != STATUS_OK)
    {
        free(line);
        return sc;
    }
    *out = line;
    return STATUS_OK;
}

static status_code_t tree_insert(Node **root, const char *word)
{
    if (root == NULL || word == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;

    while (*root != NULL)
    {
        const int cmp = strcmp(word, (*root)->word);
        if (cmp == 0)
        {
            if ((*root)->count == INT_MAX)
                return STATUS_ERROR_OVERFLOW;
            (*root)->count++;
            return STATUS_OK;
        }
        root = cmp < 0 ? &(*root)->left : &(*root)->right;
    }
    return node_create(word, 1, root);
}

static status_code_t tree_count(const Node *root, const char *word, int *count)
{
    if (word == NULL || count == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;

    *count = 0;
    while (root != NULL)
    {
        const int cmp = strcmp(word, root->word);
        if (cmp == 0)
        {
            *count = root->count;
            break;
        }
        root = cmp < 0 ? root->left : root->right;
    }
    return STATUS_OK;
}

static status_code_t tree_size(const Node *root, int *size)
{
    if (size == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;

    *size = 0;
    if (root == NULL)
        return STATUS_OK;
    int left = 0;
    int right = 0;
    tree_size(root->left, &left);
    tree_size(root->right, &right);
    *size = 1 + left + right;
    return STATUS_OK;
}

static status_code_t tree_depth(const Node *root, int *depth)
{
    if (depth == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;

    *depth = 0;
    if (root == NULL)
        return STATUS_OK;
    int left = 0;
    int right = 0;
    tree_depth(root->left, &left);
    tree_depth(root->right, &right);
    *depth = 1 + (left > right ? left : right);
    return STATUS_OK;
}

// *longest и *shortest должны быть NULL перед первым вызовом
static status_code_t find_longest_shortest(const Node *root, const Node **longest, const Node **shortest)
{
    if (longest == NULL || shortest == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;
    if (root == NULL)
        return STATUS_OK;

    if (*longest == NULL || strlen(root->word) > strlen((*longest)->word))
        *longest = root;
    if (*shortest == NULL || strlen(root->word) < strlen((*shortest)->word))
        *shortest = root;
    find_longest_shortest(root->left, longest, shortest);
    find_longest_shortest(root->right, longest, shortest);
    return STATUS_OK;
}

static status_code_t collect_nodes(const Node *root, const Node **array, int *index)
{
    if (array == NULL || index == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;
    if (root == NULL)
        return STATUS_OK;

    collect_nodes(root->left, array, index);
    array[(*index)++] = root;
    collect_nodes(root->right, array, index);
    return STATUS_OK;
}

static int compare_by_count(const void *a, const void *b)
{
    const Node *na = *(const Node *const *)a;
    const Node *nb = *(const Node *const *)b;
    return (nb->count > na->count) - (nb->count < na->count);
}

// Возвращает массив всех узлов, отсортированный по убыванию частоты.
// Массив нужно освободить вызывающему коду.
static status_code_t tree_sorted_by_count(const Node *root, const Node ***out, int *size)
{
    if (out == NULL || size == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;

    tree_size(root, size);
    if (*size == 0)
        return STATUS_ERROR_EMPTY_TREE;

    const Node **array = (const Node **)malloc(*size * sizeof(Node *));
    if (array == NULL)
        return STATUS_ERROR_OUT_OF_MEMORY;

    int index = 0;
    collect_nodes(root, array, &index);
    qsort((void *)array, *size, sizeof(Node *), compare_by_count);
    *out = array;
    return STATUS_OK;
}

static bool is_separator(const char c, const int sep_count, char *const separators[])
{
    if (c == '\n' || c == '\r')
        return true;
    for (int i = 0; i < sep_count; i++)
    {
        if (separators[i][0] == c)
            return true;
    }
    return false;
}

static status_code_t build_tree(const char *path, const int sep_count, char *const separators[], Node **root)
{
    if (path == NULL || separators == NULL || root == NULL || sep_count <= 0)
        return STATUS_ERROR_INVALID_ARGUMENT;

    FILE *file = fopen(path, "r");
    if (file == NULL)
        return STATUS_ERROR_FILE_OPEN;

    int length = 0;
    int capacity = START_CAPACITY;
    char *word = (char *)malloc(capacity);
    status_code_t sc = word == NULL ? STATUS_ERROR_OUT_OF_MEMORY : STATUS_OK;
    int c = 0;

    while (sc == STATUS_OK && c != EOF)
    {
        c = fgetc(file);
        if (c != EOF && !is_separator((char)c, sep_count, separators))
            sc = append_char(&word, &length, &capacity, (char)c);
        else if (length > 0)
        {
            sc = tree_insert(root, word);
            length = 0;
        }
    }

    free(word);
    fclose(file);
    return sc;
}

// Формат: прямой обход, узел - строка "+count:word", пустой потомок - строка "-"
static status_code_t save_node(FILE *file, const Node *node)
{
    if (file == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;

    if (node == NULL)
        return fprintf(file, "-\n") < 0 ? STATUS_ERROR_FILE_WRITE : STATUS_OK;

    if (fprintf(file, "+%d:%s\n", node->count, node->word) < 0)
        return STATUS_ERROR_FILE_WRITE;
    status_code_t sc = save_node(file, node->left);
    if (sc == STATUS_OK)
        sc = save_node(file, node->right);
    return sc;
}

static status_code_t tree_save(const char *path, const Node *root)
{
    if (path == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;

    FILE *file = fopen(path, "w");
    if (file == NULL)
        return STATUS_ERROR_FILE_OPEN;
    status_code_t sc = save_node(file, root);
    if (fclose(file) != 0 && sc == STATUS_OK)
        sc = STATUS_ERROR_FILE_WRITE;
    return sc;
}

static status_code_t load_node(FILE *file, Node **out)
{
    if (file == NULL || out == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;

    char *line = NULL;
    *out = NULL;
    status_code_t sc = read_dynamic_line(file, &line);
    if (sc == STATUS_ERROR_END_OF_INPUT)
        return STATUS_ERROR_FILE_FORMAT;
    if (sc != STATUS_OK)
        return sc;

    int count = 0;
    int start = 0;
    if (strcmp(line, "-") == 0)
        sc = STATUS_OK;
    else if (sscanf(line, "+%d:%n", &count, &start) != 1 || start == 0 || count <= 0 || line[start] == '\0')
        sc = STATUS_ERROR_FILE_FORMAT;
    else
        sc = node_create(line + start, count, out);
    free(line);

    if (sc == STATUS_OK && *out != NULL)
        sc = load_node(file, &(*out)->left);
    if (sc == STATUS_OK && *out != NULL)
        sc = load_node(file, &(*out)->right);
    return sc;
}

// Проверяет, что дерево является деревом поиска: min < word < max (NULL - нет границы)
static status_code_t check_bst(const Node *node, const char *min, const char *max)
{
    if (node == NULL)
        return STATUS_OK;
    if ((min != NULL && strcmp(node->word, min) <= 0) || (max != NULL && strcmp(node->word, max) >= 0))
        return STATUS_ERROR_FILE_FORMAT;

    const status_code_t sc = check_bst(node->left, min, node->word);
    return sc == STATUS_OK ? check_bst(node->right, node->word, max) : sc;
}

static status_code_t tree_load(const char *path, Node **root)
{
    if (path == NULL || root == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;

    FILE *file = fopen(path, "r");
    if (file == NULL)
        return STATUS_ERROR_FILE_OPEN;

    status_code_t sc = load_node(file, root);
    if (sc == STATUS_OK && fgetc(file) != EOF)
        sc = STATUS_ERROR_FILE_FORMAT;
    if (sc == STATUS_OK)
        sc = check_bst(*root, NULL, NULL);
    fclose(file);

    if (sc != STATUS_OK)
    {
        tree_free(*root);
        *root = NULL;
    }
    return sc;
}

static void print_tree(const Node *node, const int level)
{
    if (node == NULL)
        return;
    print_tree(node->right, level + 1);
    for (int i = 0; i < level; i++)
        printf("    ");
    printf("%s (%d)\n", node->word, node->count);
    print_tree(node->left, level + 1);
}

static status_code_t read_line(const char *prompt, char **out)
{
    if (prompt == NULL || out == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;
    printf("%s", prompt);
    *out = NULL;
    return read_dynamic_line(stdin, out);
}

static const char *status_to_string(const status_code_t sc)
{
    switch (sc)
    {
    case STATUS_ERROR_OUT_OF_MEMORY:
        return "недостаточно памяти";
    case STATUS_ERROR_INVALID_ARGUMENT:
        return "неверный аргумент";
    case STATUS_ERROR_FILE_OPEN:
        return "не удается открыть файл";
    case STATUS_ERROR_FILE_WRITE:
        return "ошибка записи в файл";
    case STATUS_ERROR_FILE_FORMAT:
        return "некорректный формат файла с деревом";
    case STATUS_ERROR_END_OF_INPUT:
        return "ввод закончился";
    case STATUS_ERROR_EMPTY_TREE:
        return "дерево пустое";
    case STATUS_ERROR_OVERFLOW:
        return "переполнение";
    default:
        return "успешно";
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Использование: %s <файл> <сепаратор1> [<сепаратор2> ...]\n", argv[0]);
        return STATUS_ERROR_INVALID_ARGUMENT;
    }
    for (int i = 2; i < argc; i++)
    {
        if (strlen(argv[i]) != 1)
        {
            printf("Ошибка: сепаратор '%s' должен быть одним символом\n", argv[i]);
            return STATUS_ERROR_INVALID_ARGUMENT;
        }
    }

    Node *root = NULL;
    status_code_t sc = build_tree(argv[1], argc - 2, argv + 2, &root);
    if (sc != STATUS_OK)
    {
        printf("Ошибка: %s\n", status_to_string(sc));
        tree_free(root);
        return sc;
    }
    int size = 0;
    tree_size(root, &size);
    printf("Различных слов: %d\n", size);

    bool running = true;
    while (running)
    {
        printf("\n1. Сколько раз встречается слово\n2. Первые n самых частых слов\n");
        printf("3. Самое длинное и самое короткое слово\n4. Глубина дерева\n5. Показать дерево\n");
        printf("6. Сохранить дерево в файл\n7. Загрузить дерево из файла\n0. Выход\n");

        char *line = NULL;
        if (read_line("> ", &line) != STATUS_OK)
            break;

        int choice = -1;
        char extra;
        if (sscanf(line, "%d %c", &choice, &extra) != 1)
            choice = -1;
        free(line);

        char *input = NULL;
        int n = 0;
        int count = 0;
        int depth = 0;
        const Node *longest = NULL;
        const Node *shortest = NULL;
        const Node **array = NULL;
        Node *loaded = NULL;
        sc = STATUS_OK;

        switch (choice)
        {
        case 0:
            running = false;
            break;

        case 1:
            sc = read_line("Слово: ", &input);
            if (sc == STATUS_OK && input[0] == '\0')
                sc = STATUS_ERROR_INVALID_ARGUMENT;
            if (sc == STATUS_OK)
                sc = tree_count(root, input, &count);
            if (sc == STATUS_OK)
                printf("Слово \"%s\" встречается %d раз\n", input, count);
            break;

        case 2:
            sc = read_line("n: ", &input);
            if (sc == STATUS_OK && (sscanf(input, "%d %c", &n, &extra) != 1 || n <= 0))
                sc = STATUS_ERROR_INVALID_ARGUMENT;
            if (sc == STATUS_OK)
                sc = tree_sorted_by_count(root, &array, &size);
            if (sc == STATUS_OK)
            {
                for (int i = 0; i < n && i < size; i++)
                    printf("%d. %s - %d\n", i + 1, array[i]->word, array[i]->count);
            }
            free((void *)array);
            break;

        case 3:
            sc = find_longest_shortest(root, &longest, &shortest);
            if (sc == STATUS_OK && longest == NULL)
                sc = STATUS_ERROR_EMPTY_TREE;
            if (sc == STATUS_OK)
                printf("Самое длинное: %s\nСамое короткое: %s\n", longest->word, shortest->word);
            break;

        case 4:
            sc = tree_depth(root, &depth);
            if (sc == STATUS_OK)
                printf("Глубина дерева: %d\n", depth);
            break;

        case 5:
            print_tree(root, 0);
            break;

        case 6:
            sc = read_line("Путь к файлу: ", &input);
            if (sc == STATUS_OK)
                sc = tree_save(input, root);
            if (sc == STATUS_OK)
                printf("Дерево сохранено\n");
            break;

        case 7:
            sc = read_line("Путь к файлу: ", &input);
            if (sc == STATUS_OK)
                sc = tree_load(input, &loaded);
            if (sc == STATUS_OK)
            {
                tree_free(root);
                root = loaded;
                tree_size(root, &size);
                printf("Загружено слов: %d\n", size);
            }
            break;

        default:
            printf("Неизвестная команда.\n");
            break;
        }

        free(input);
        if (sc != STATUS_OK)
            printf("Ошибка: %s\n", status_to_string(sc));
    }

    tree_free(root);
    return STATUS_OK;
}
