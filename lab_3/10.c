#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define START_CAPACITY 16

typedef enum
{
    STATUS_OK,
    STATUS_ERROR_OUT_OF_MEMORY,
    STATUS_ERROR_INVALID_ARGUMENT,
    STATUS_ERROR_FILE_OPEN,
    STATUS_ERROR_SYNTAX,
    STATUS_ERROR_END_OF_FILE
} status_code_t;

typedef struct Node
{
    char *name;
    struct Node *child;
    struct Node *sibling;
} Node;

static void tree_free(Node *node)
{
    while (node != NULL)
    {
        Node *next = node->sibling;
        tree_free(node->child);
        free(node->name);
        free(node);
        node = next;
    }
}

static status_code_t read_line(FILE *file, char **out)
{
    int length = 0;
    int capacity = START_CAPACITY;
    char *line = (char *)malloc(capacity);
    if (line == NULL)
        return STATUS_ERROR_OUT_OF_MEMORY;

    int c;
    while ((c = fgetc(file)) != EOF && c != '\n')
    {
        if (length + 1 == capacity)
        {
            capacity *= 2;
            char *new_line = (char *)realloc(line, capacity);
            if (new_line == NULL)
            {
                free(line);
                return STATUS_ERROR_OUT_OF_MEMORY;
            }
            line = new_line;
        }
        if (c != '\r')
            line[length++] = (char)c;
    }
    line[length] = '\0';

    if (c == EOF && length == 0)
    {
        free(line);
        return STATUS_ERROR_END_OF_FILE;
    }
    *out = line;
    return STATUS_OK;
}

static void skip_spaces(const char **p)
{
    while (isspace((unsigned char)**p))
        (*p)++;
}

static bool is_name_char(const char c)
{
    return c != '\0' && c != '(' && c != ')' && c != ',' && !isspace((unsigned char)c);
}

static status_code_t parse_node(const char **p, Node **out)
{
    *out = NULL;
    skip_spaces(p);

    const char *start = *p;
    while (is_name_char(**p))
        (*p)++;
    const int length = (int)(*p - start);
    if (length == 0)
        return STATUS_ERROR_SYNTAX;

    Node *node = (Node *)malloc(sizeof(Node));
    char *name = (char *)malloc(length + 1);
    if (node == NULL || name == NULL)
    {
        free(node);
        free(name);
        return STATUS_ERROR_OUT_OF_MEMORY;
    }
    memcpy(name, start, length);
    name[length] = '\0';
    node->name = name;
    node->child = NULL;
    node->sibling = NULL;
    *out = node;

    skip_spaces(p);
    if (**p != '(')
        return STATUS_OK;
    (*p)++;

    Node **last = &node->child;
    while (true)
    {
        const status_code_t sc = parse_node(p, last);
        if (sc != STATUS_OK)
            return sc;
        Node *just_added = *last;
        Node **next_slot = &just_added->sibling;
        last = next_slot;

        skip_spaces(p);
        if (**p == ')')
        {
            (*p)++;
            return STATUS_OK;
        }
        if (**p != ',')
            return STATUS_ERROR_SYNTAX;
        (*p)++;
    }
}

static status_code_t build_tree(const char *expression, Node **root)
{
    const char *p = expression;
    status_code_t sc = parse_node(&p, root);
    if (sc == STATUS_OK)
    {
        skip_spaces(&p);
        if (*p != '\0')
            sc = STATUS_ERROR_SYNTAX;
    }
    if (sc != STATUS_OK)
    {
        tree_free(*root);
        *root = NULL;
    }
    return sc;
}

static status_code_t print_node(FILE *out, const Node *node, const char *prefix, const bool is_last, const bool is_root)
{
    if (is_root)
        fprintf(out, "%s\n", node->name);
    else
        fprintf(out, "%s%s%s\n", prefix, is_last ? "`-- " : "|-- ", node->name);

    const char *addition = is_root ? "" : (is_last ? "    " : "|   ");
    char *child_prefix = (char *)malloc(strlen(prefix) + strlen(addition) + 1);
    if (child_prefix == NULL)
        return STATUS_ERROR_OUT_OF_MEMORY;
    strcpy(child_prefix, prefix);
    strcat(child_prefix, addition);

    status_code_t sc = STATUS_OK;
    for (const Node *child = node->child; child != NULL && sc == STATUS_OK; child = child->sibling)
        sc = print_node(out, child, child_prefix, child->sibling == NULL, false);

    free(child_prefix);
    return sc;
}

static status_code_t process_files(const char *input_path, const char *output_path, int *out_count, int *out_errors)
{
    FILE *input = fopen(input_path, "r");
    if (input == NULL)
        return STATUS_ERROR_FILE_OPEN;
    FILE *output = fopen(output_path, "w");
    if (output == NULL)
    {
        fclose(input);
        return STATUS_ERROR_FILE_OPEN;
    }

    *out_count = 0;
    *out_errors = 0;
    status_code_t sc = STATUS_OK;
    while (sc == STATUS_OK)
    {
        char *line = NULL;
        sc = read_line(input, &line);
        if (sc != STATUS_OK)
            break;

        const char *p = line;
        skip_spaces(&p);
        if (*p == '\0')
        {
            free(line);
            continue;
        }

        (*out_count)++;
        fprintf(output, "Выражение %d: %s\n", *out_count, p);

        Node *root = NULL;
        sc = build_tree(line, &root);
        if (sc == STATUS_ERROR_SYNTAX)
        {
            fprintf(output, "Ошибка: некорректное скобочное выражение\n");
            (*out_errors)++;
            sc = STATUS_OK;
        }
        else if (sc == STATUS_OK)
        {
            sc = print_node(output, root, "", true, true);
        }
        fprintf(output, "\n");

        tree_free(root);
        free(line);
    }

    fclose(input);
    fclose(output);
    return sc == STATUS_ERROR_END_OF_FILE ? STATUS_OK : sc;
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
    case STATUS_ERROR_SYNTAX:
        return "синтаксическая ошибка";
    default:
        return "успешно";
    }
}

static status_code_t check_same_files(const char *path1, const char *path2, int *out_same)
{
    if (path1 == NULL || path2 == NULL || out_same == NULL)
    {
        return STATUS_ERROR_INVALID_ARGUMENT;
    }

    char *rp1 = realpath(path1, NULL);
    if (rp1 == NULL)
    {
        return STATUS_ERROR_FILE_OPEN;
    }

    char *rp2 = realpath(path2, NULL);
    if (rp2 != NULL)
    {
        *out_same = (strcmp(rp1, rp2) == 0) ? 1 : 0;
        free(rp2);
    }
    else
    {
        *out_same = (strcmp(path1, path2) == 0) ? 1 : 0;
    }

    free(rp1);
    return STATUS_OK;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Использование: %s <входной файл> <выходной файл>\n", argv[0]);
        return STATUS_ERROR_INVALID_ARGUMENT;
    }

    int same = 0;
    const status_code_t check_sc = check_same_files(argv[1], argv[2], &same);
    if (check_sc != STATUS_OK)
    {
        printf("Ошибка: %s\n", status_to_string(check_sc));
        return check_sc;
    }
    if (same)
    {
        printf("Ошибка: входной и выходной файлы должны различаться\n");
        return STATUS_ERROR_INVALID_ARGUMENT;
    }

    int count = 0;
    int errors = 0;
    const status_code_t sc = process_files(argv[1], argv[2], &count, &errors);
    if (sc != STATUS_OK)
    {
        printf("Ошибка: %s\n", status_to_string(sc));
        return sc;
    }
    return STATUS_OK;
}