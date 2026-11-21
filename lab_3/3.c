#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node
{
    long weight;
    int byte;
    struct Node *left;
    struct Node *right;
} Node;

typedef struct
{
    Node *data[256];
    int size;
} Heap;

static long byte_in_file[256];
static char codes[256][257];

Node *new_node(long weight, int byte, Node *left, Node *right)
{
    Node *node = malloc(sizeof(Node));
    if (node == NULL)
        return NULL;
    node->weight = weight;
    node->byte = byte;
    node->left = left;
    node->right = right;
    return node;
}

void free_tree(Node *node)
{
    if (node == NULL)
        return;
    free_tree(node->left);
    free_tree(node->right);
    free(node);
}

void swap(Node **x, Node **y)
{
    Node *tmp = *x;
    *x = *y;
    *y = tmp;
}

void heap_push(Heap *heap, Node *node)
{
    int i = heap->size;
    heap->data[i] = node;
    heap->size++;

    while (i > 0)
    {
        int parent = (i - 1) / 2;
        if (heap->data[parent]->weight <= heap->data[i]->weight)
            break;
        swap(&heap->data[parent], &heap->data[i]);
        i = parent;
    }
}

Node *heap_pop(Heap *heap)
{
    Node *top = heap->data[0];
    heap->size--;
    heap->data[0] = heap->data[heap->size];

    int i = 0;
    while (1)
    {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int min = i;
        if (left < heap->size && heap->data[left]->weight < heap->data[min]->weight)
            min = left;
        if (right < heap->size && heap->data[right]->weight < heap->data[min]->weight)
            min = right;
        if (min == i)
            break;
        swap(&heap->data[i], &heap->data[min]);
        i = min;
    }
    return top;
}

void heap_free_all(Heap *heap)
{
    for (int i = 0; i < heap->size; i++)
        free_tree(heap->data[i]);
    heap->size = 0;
}

void make_codes(Node *node, char path[], int depth)
{
    if (node->left == NULL)
    {
        path[depth] = '\0';
        strcpy(codes[node->byte], path);
        return;
    }
    path[depth] = '0';
    make_codes(node->left, path, depth + 1);
    path[depth] = '1';
    make_codes(node->right, path, depth + 1);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Использование: %s <файл>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL)
    {
        printf("Не удалось открыть файл\n");
        return 1;
    }
    long file_size = 0;
    int c;
    while ((c = fgetc(file)) != EOF)
    {
        byte_in_file[c]++;
        file_size++;
    }
    if (ferror(file))
    {
        printf("Ошибка чтения файла\n");
        fclose(file);
        return 1;
    }
    fclose(file);

    Heap heap;
    heap.size = 0;
    for (int b = 0; b < 256; b++)
    {
        if (byte_in_file[b] > 0)
        {
            Node *leaf = new_node(byte_in_file[b], b, NULL, NULL);
            if (leaf == NULL)
            {
                printf("Не хватает памяти\n");
                heap_free_all(&heap);
                return 1;
            }
            heap_push(&heap, leaf);
        }
    }

    while (heap.size > 1)
    {
        Node *a = heap_pop(&heap);
        Node *b = heap_pop(&heap);
        Node *parent = new_node(a->weight + b->weight, 0, a, b);
        if (parent == NULL)
        {
            printf("Не хватает памяти\n");
            free_tree(a);
            free_tree(b);
            heap_free_all(&heap);
            return 1;
        }
        heap_push(&heap, parent);
    }

    if (heap.size == 1)
    {
        Node *root = heap_pop(&heap);
        if (root->left == NULL)
        {
            strcpy(codes[root->byte], "0");
        }
        else
        {
            char path[257];
            make_codes(root, path, 0);
        }
        free_tree(root);
    }

    long bits = 0;
    printf("Байт   | Частота      | Код\n");
    for (int b = 0; b < 256; b++)
    {
        if (byte_in_file[b] > 0)
        {
            printf("%-6d | %-12ld | %s\n", b, byte_in_file[b], codes[b]);
            bits += byte_in_file[b] * (long)strlen(codes[b]);
        }
    }
    long bytes = (bits + 7) / 8;
    double savings = 0.0;
    if (file_size > 0)
        savings = 100.0 * (1.0 - (double)bytes / file_size);

    printf("\nРазмер исходного файла: %ld байт\n", file_size);
    printf("Закодированные данные:  %ld бит (%ld байт)\n", bits, bytes);
    printf("Экономия:               %.2f%%\n", savings);
    return 0;
}
