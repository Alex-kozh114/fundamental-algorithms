#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define STR_SIZE 64
#define LINE_SIZE 512
#define EPS 1e-9

typedef enum
{
    STATUS_OK,
    STATUS_ERROR_NULL_POINTER,
    STATUS_ERROR_OUT_OF_MEMORY,
    STATUS_ERROR_FILE_OPEN,
    STATUS_ERROR_INVALID_FORMAT,
    STATUS_ERROR_NOT_FOUND,
    STATUS_ERROR_NOTHING_TO_UNDO
} status_code_t;

typedef struct
{
    char surname[STR_SIZE];
    char name[STR_SIZE];
    char patronymic[STR_SIZE];
    int day;
    int month;
    int year;
    char gender;
    double income;
} Liver;

typedef struct Node
{
    Liver data;
    struct Node *next;
} Node;

typedef enum
{
    CHANGE_ADD,
    CHANGE_DELETE,
    CHANGE_EDIT
} change_type_t;

typedef struct
{
    change_type_t type;
    Liver old_data;
    Liver new_data;
} Change;

typedef struct
{
    Change *items;
    int count;
    int capacity;
} History;

static bool is_latin(const char *s, const bool allow_empty)
{
    if (s[0] == '\0')
        return allow_empty;
    for (int i = 0; s[i] != '\0'; i++)
    {
        if (!((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z')))
            return false;
    }
    return true;
}

static bool parse_date(const char *s, int *day, int *month, int *year)
{
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    char extra;
    if (sscanf(s, "%d.%d.%d%c", day, month, year, &extra) != 3)
        return false;
    if (*year < 1900 || *year > 2026 || *month < 1 || *month > 12)
        return false;
    if ((*year % 4 == 0 && *year % 100 != 0) || *year % 400 == 0)
        days[1] = 29;
    return *day >= 1 && *day <= days[*month - 1];
}

static bool parse_income(const char *s, const double eps, double *income)
{
    char extra;
    if (sscanf(s, "%lf%c", income, &extra) != 1 || !isfinite(*income) || *income < -eps)
        return false;
    if (fabs(*income) < eps)
        *income = 0;
    return true;
}

static status_code_t parse_liver(const char *line, Liver *out)
{
    if (!line || !out)
        return STATUS_ERROR_NULL_POINTER;

    char f[6][LINE_SIZE];
    char extra[2];
    const int n = sscanf(line, "%511s %511s %511s %511s %511s %511s %1s", f[0], f[1], f[2], f[3], f[4], f[5], extra);
    if (n != 5 && n != 6)
        return STATUS_ERROR_INVALID_FORMAT;
    for (int i = 0; i < n; i++)
    {
        if (strlen(f[i]) >= STR_SIZE)
            return STATUS_ERROR_INVALID_FORMAT;
    }

    const int k = n == 6 ? 1 : 0;
    strcpy(out->surname, f[0]);
    strcpy(out->name, f[1]);
    strcpy(out->patronymic, k ? f[2] : "");

    if (!is_latin(out->surname, false) || !is_latin(out->name, false) || !is_latin(out->patronymic, true))
        return STATUS_ERROR_INVALID_FORMAT;
    if (!parse_date(f[2 + k], &out->day, &out->month, &out->year))
        return STATUS_ERROR_INVALID_FORMAT;
    if (strcmp(f[3 + k], "M") != 0 && strcmp(f[3 + k], "W") != 0)
        return STATUS_ERROR_INVALID_FORMAT;
    out->gender = f[3 + k][0];
    if (!parse_income(f[4 + k], EPS, &out->income))
        return STATUS_ERROR_INVALID_FORMAT;
    return STATUS_OK;
}

static int date_value(const Liver *l)
{
    return l->year * 10000 + l->month * 100 + l->day;
}

static bool same_liver(const Liver *a, const Liver *b, const double eps)
{
    return strcmp(a->surname, b->surname) == 0 && strcmp(a->name, b->name) == 0 &&
           strcmp(a->patronymic, b->patronymic) == 0 && date_value(a) == date_value(b) &&
           a->gender == b->gender && fabs(a->income - b->income) < eps;
}

static bool matches(const Liver *l, const Liver *q, const double eps)
{
    if (q->surname[0] != '\0' && strcmp(l->surname, q->surname) != 0)
        return false;
    if (q->name[0] != '\0' && strcmp(l->name, q->name) != 0)
        return false;
    if (q->patronymic[0] != '\0' && strcmp(l->patronymic, q->patronymic) != 0)
        return false;
    if (q->year != 0 && date_value(l) != date_value(q))
        return false;
    if (q->gender != '\0' && l->gender != q->gender)
        return false;
    if (q->income > -eps && fabs(l->income - q->income) >= eps)
        return false;
    return true;
}

static status_code_t list_insert(Node **head, const Liver *liver)
{
    if (!head || !liver)
        return STATUS_ERROR_NULL_POINTER;
    Node *node = (Node *)malloc(sizeof(Node));
    if (!node)
        return STATUS_ERROR_OUT_OF_MEMORY;
    node->data = *liver;

    if (*head == NULL || date_value(&(*head)->data) < date_value(liver))
    {
        node->next = *head;
        *head = node;
        return STATUS_OK;
    }

    Node *cur = *head;
    while (cur->next != NULL && date_value(&cur->next->data) >= date_value(liver))
        cur = cur->next;
    node->next = cur->next;
    cur->next = node;
    return STATUS_OK;
}

static status_code_t list_remove_at(Node **head, const int index, Liver *removed)
{
    if (!head || !removed)
        return STATUS_ERROR_NULL_POINTER;
    Node *prev = NULL;
    Node *cur = *head;
    for (int i = 0; cur != NULL && i < index; i++)
    {
        prev = cur;
        cur = cur->next;
    }
    if (index < 0 || cur == NULL)
        return STATUS_ERROR_NOT_FOUND;

    if (prev == NULL)
        *head = cur->next;
    else
        prev->next = cur->next;
    *removed = cur->data;
    free(cur);
    return STATUS_OK;
}

static status_code_t list_remove_same(Node **head, const Liver *liver, const double eps)
{
    if (!head || !liver)
        return STATUS_ERROR_NULL_POINTER;
    int index = 0;
    for (Node *cur = *head; cur != NULL; cur = cur->next, index++)
    {
        if (same_liver(&cur->data, liver, eps))
        {
            Liver removed;
            return list_remove_at(head, index, &removed);
        }
    }
    return STATUS_ERROR_NOT_FOUND;
}

static void list_free(Node *head)
{
    while (head != NULL)
    {
        Node *next = head->next;
        free(head);
        head = next;
    }
}

static status_code_t history_add(History *h, const change_type_t type, const Liver *old_data, const Liver *new_data)
{
    if (!h)
        return STATUS_ERROR_NULL_POINTER;
    if (h->count == h->capacity)
    {
        const int new_capacity = h->capacity == 0 ? 8 : h->capacity * 2;
        Change *items = (Change *)realloc(h->items, new_capacity * sizeof(Change));
        if (!items)
            return STATUS_ERROR_OUT_OF_MEMORY;
        h->items = items;
        h->capacity = new_capacity;
    }

    Change *c = &h->items[h->count];
    memset(c, 0, sizeof(Change));
    c->type = type;
    if (old_data)
        c->old_data = *old_data;
    if (new_data)
        c->new_data = *new_data;
    h->count++;
    return STATUS_OK;
}

static status_code_t undo(History *h, Node **head, int *undone, const double eps)
{
    if (!h || !head || !undone)
        return STATUS_ERROR_NULL_POINTER;
    const int n = h->count / 2;
    if (n == 0)
        return STATUS_ERROR_NOTHING_TO_UNDO;

    for (int i = 0; i < n; i++)
    {
        const Change *c = &h->items[h->count - 1];
        status_code_t sc = STATUS_OK;
        if (c->type == CHANGE_ADD || c->type == CHANGE_EDIT)
            sc = list_remove_same(head, &c->new_data, eps);
        if (sc == STATUS_OK && (c->type == CHANGE_DELETE || c->type == CHANGE_EDIT))
            sc = list_insert(head, &c->old_data);
        if (sc != STATUS_OK)
            return sc;
        h->count--;
    }

    h->count = 0;
    *undone = n;
    return STATUS_OK;
}

static status_code_t load_file(const char *path, Node **head, int *line_number)
{
    if (!path || !head || !line_number)
        return STATUS_ERROR_NULL_POINTER;
    FILE *file = fopen(path, "r");
    if (!file)
        return STATUS_ERROR_FILE_OPEN;

    char line[LINE_SIZE];
    *line_number = 0;
    while (fgets(line, sizeof(line), file))
    {
        (*line_number)++;
        char check[2];
        if (sscanf(line, "%1s", check) != 1)
            continue;

        Liver liver;
        status_code_t sc = parse_liver(line, &liver);
        if (sc == STATUS_OK)
            sc = list_insert(head, &liver);
        if (sc != STATUS_OK)
        {
            fclose(file);
            return sc;
        }
    }
    fclose(file);
    return STATUS_OK;
}

static void print_liver(FILE *out, const Liver *l)
{
    fprintf(out, "%s %s ", l->surname, l->name);
    if (l->patronymic[0] != '\0')
        fprintf(out, "%s ", l->patronymic);
    fprintf(out, "%02d.%02d.%04d %c %.2f\n", l->day, l->month, l->year, l->gender, l->income);
}

static status_code_t save_file(const char *path, const Node *head)
{
    if (!path)
        return STATUS_ERROR_NULL_POINTER;
    FILE *file = fopen(path, "w");
    if (!file)
        return STATUS_ERROR_FILE_OPEN;
    for (const Node *cur = head; cur != NULL; cur = cur->next)
        print_liver(file, &cur->data);
    fclose(file);
    return STATUS_OK;
}

static bool read_line(const char *prompt, char *buf, const int size)
{
    printf("%s", prompt);
    if (!fgets(buf, size, stdin))
        return false;
    buf[strcspn(buf, "\r\n")] = '\0';
    return true;
}

static status_code_t read_query(Liver *q)
{
    char line[LINE_SIZE];
    memset(q, 0, sizeof(Liver));
    q->income = -1;
    printf("Вводите параметры поиска (пустая строка - не важно)\n");

    if (!read_line("Фамилия: ", line, sizeof(line)) || strlen(line) >= STR_SIZE || !is_latin(line, true))
        return STATUS_ERROR_INVALID_FORMAT;
    strcpy(q->surname, line);
    if (!read_line("Имя: ", line, sizeof(line)) || strlen(line) >= STR_SIZE || !is_latin(line, true))
        return STATUS_ERROR_INVALID_FORMAT;
    strcpy(q->name, line);
    if (!read_line("Отчество: ", line, sizeof(line)) || strlen(line) >= STR_SIZE || !is_latin(line, true))
        return STATUS_ERROR_INVALID_FORMAT;
    strcpy(q->patronymic, line);

    if (!read_line("Дата рождения (дд.мм.гггг): ", line, sizeof(line)))
        return STATUS_ERROR_INVALID_FORMAT;
    if (line[0] != '\0' && !parse_date(line, &q->day, &q->month, &q->year))
        return STATUS_ERROR_INVALID_FORMAT;

    if (!read_line("Пол (M/W): ", line, sizeof(line)))
        return STATUS_ERROR_INVALID_FORMAT;
    if (line[0] != '\0' && strcmp(line, "M") != 0 && strcmp(line, "W") != 0)
        return STATUS_ERROR_INVALID_FORMAT;
    q->gender = line[0];

    if (!read_line("Доход: ", line, sizeof(line)))
        return STATUS_ERROR_INVALID_FORMAT;
    if (line[0] != '\0' && !parse_income(line, EPS, &q->income))
        return STATUS_ERROR_INVALID_FORMAT;
    return STATUS_OK;
}

static const char *status_to_string(const status_code_t sc)
{
    switch (sc)
    {
    case STATUS_OK:
        return "Готово";
    case STATUS_ERROR_NULL_POINTER:
        return "Передан нулевой указатель";
    case STATUS_ERROR_OUT_OF_MEMORY:
        return "Недостаточно памяти";
    case STATUS_ERROR_FILE_OPEN:
        return "Не удается открыть файл";
    case STATUS_ERROR_INVALID_FORMAT:
        return "Некорректные данные";
    case STATUS_ERROR_NOT_FOUND:
        return "Житель не найден";
    case STATUS_ERROR_NOTHING_TO_UNDO:
        return "Нечего отменять";
    default:
        return "Неизвестная ошибка";
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Использование: %s <файл с жителями>\n", argv[0]);
        return 1;
    }

    Node *head = NULL;
    History history = {NULL, 0, 0};
    int bad_line = 0;
    status_code_t sc = load_file(argv[1], &head, &bad_line);
    if (sc != STATUS_OK)
    {
        if (sc == STATUS_ERROR_INVALID_FORMAT)
            printf("Ошибка: %s (строка %d)\n", status_to_string(sc), bad_line);
        else
            printf("Ошибка: %s\n", status_to_string(sc));
        list_free(head);
        return sc;
    }

    char line[LINE_SIZE];
    bool running = true;
    while (running)
    {
        printf("\n1. Показать всех\n2. Найти\n3. Добавить\n4. Изменить\n5. Удалить\n");
        printf("6. Сохранить в файл\n7. Отменить последние N/2 изменений (N = %d)\n0. Выход\n", history.count);
        if (!read_line("> ", line, sizeof(line)))
            break;

        int choice = -1;
        char extra;
        if (sscanf(line, "%d %c", &choice, &extra) != 1)
            choice = -1;

        Liver liver;
        Liver old_liver;
        int index = 0;
        int count = 0;
        sc = STATUS_OK;

        switch (choice)
        {
        case 0:
            running = false;
            break;

        case 1:
            for (Node *cur = head; cur != NULL; cur = cur->next)
            {
                printf("%d. ", ++count);
                print_liver(stdout, &cur->data);
            }
            if (count == 0)
                printf("Список пуст.\n");
            break;

        case 2:
            sc = read_query(&liver);
            if (sc != STATUS_OK)
                break;
            for (Node *cur = head; cur != NULL; cur = cur->next)
            {
                if (matches(&cur->data, &liver, EPS))
                {
                    print_liver(stdout, &cur->data);
                    count++;
                }
            }
            printf("Найдено: %d\n", count);
            break;

        case 3:
            printf("Фамилия Имя [Отчество] дд.мм.гггг M/W Доход\n");
            if (!read_line("> ", line, sizeof(line)))
                break;
            sc = parse_liver(line, &liver);
            if (sc == STATUS_OK)
                sc = list_insert(&head, &liver);
            if (sc == STATUS_OK)
                sc = history_add(&history, CHANGE_ADD, NULL, &liver);
            break;

        case 4:
        case 5:
            if (!read_line("Номер жителя: ", line, sizeof(line)))
                break;
            if (sscanf(line, "%d %c", &index, &extra) != 1)
            {
                sc = STATUS_ERROR_INVALID_FORMAT;
                break;
            }
            if (choice == 4)
            {
                printf("Новые данные: Фамилия Имя [Отчество] дд.мм.гггг M/W Доход\n");
                if (!read_line("> ", line, sizeof(line)))
                    break;
                sc = parse_liver(line, &liver);
                if (sc != STATUS_OK)
                    break;
            }
            sc = list_remove_at(&head, index - 1, &old_liver);
            if (sc == STATUS_OK && choice == 4)
                sc = list_insert(&head, &liver);
            if (sc == STATUS_OK)
                sc = history_add(&history, choice == 4 ? CHANGE_EDIT : CHANGE_DELETE, &old_liver,
                                 choice == 4 ? &liver : NULL);
            break;

        case 6:
            if (!read_line("Путь к файлу: ", line, sizeof(line)))
                break;
            sc = save_file(line, head);
            break;

        case 7:
            sc = undo(&history, &head, &count, EPS);
            if (sc == STATUS_OK)
                printf("Отменено изменений: %d\n", count);
            break;

        default:
            printf("Неизвестная команда.\n");
            break;
        }

        if (sc != STATUS_OK)
            printf("Ошибка: %s\n", status_to_string(sc));
        else if (choice >= 3 && choice <= 6)
            printf("Готово.\n");
    }

    list_free(head);
    free(history.items);
    return 0;
}