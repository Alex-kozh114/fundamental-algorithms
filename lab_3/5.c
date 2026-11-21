#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>

#define EXAMS_COUNT 5
#define MIN_GRADE 2
#define MAX_GRADE 5
#define LINE_SIZE 1024
#define FIELDS_COUNT (4 + EXAMS_COUNT)
#define AVERAGE_EPS 1e-9

typedef enum
{
    STATUS_OK,
    STATUS_ERROR_NULL_POINTER,
    STATUS_ERROR_OUT_OF_MEMORY,
    STATUS_ERROR_INVALID_ARGUMENT,
    STATUS_ERROR_FILE_OPEN,
    STATUS_ERROR_FILE_READ,
    STATUS_ERROR_INVALID_FORMAT,
    STATUS_ERROR_DUPLICATE_ID,
    STATUS_ERROR_NOT_FOUND,
    STATUS_ERROR_EMPTY_COLLECTION,
    STATUS_ERROR_INPUT_END
} status_code_t;

typedef struct
{
    unsigned int id;
    char *name;
    char *surname;
    char *group;
    unsigned char *grades;
} Student;

typedef bool (*student_match_t)(const Student *, const void *);
typedef int (*student_compare_t)(const void *, const void *);

static void student_destroy(Student *student)
{
    if (!student)
        return;
    free(student->name);
    free(student->surname);
    free(student->group);
    free(student->grades);
    memset(student, 0, sizeof(*student));
}

static void students_free(Student *students, const size_t count)
{
    if (!students)
        return;
    for (size_t i = 0; i < count; ++i)
    {
        student_destroy(&students[i]);
    }
    free(students);
}

static char *string_duplicate(const char *src)
{
    const size_t len = strlen(src);
    char *copy = (char *)malloc(len + 1);
    if (copy)
        memcpy(copy, src, len + 1);
    return copy;
}

static bool is_latin_word(const char *s)
{
    if (!s || *s == '\0')
        return false;
    for (; *s != '\0'; ++s)
    {
        if (!((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z')))
            return false;
    }
    return true;
}

static bool parse_unsigned(const char *s, const unsigned long long max, unsigned long long *out)
{
    if (!s || !out || *s == '\0')
        return false;

    unsigned long long value = 0;
    for (; *s != '\0'; ++s)
    {
        if (!isdigit((unsigned char)*s))
            return false;
        const unsigned long long digit = (unsigned long long)(*s - '0');
        if (digit > max || value > (max - digit) / 10)
            return false;
        value = value * 10 + digit;
    }
    *out = value;
    return true;
}

static size_t split_tokens(char *line, char *tokens[], const size_t max_tokens)
{
    size_t count = 0;
    char *p = line;
    while (*p != '\0')
    {
        while (*p != '\0' && isspace((unsigned char)*p))
            ++p;
        if (*p == '\0')
            break;
        if (count == max_tokens)
            return max_tokens + 1;
        tokens[count++] = p;
        while (*p != '\0' && !isspace((unsigned char)*p))
            ++p;
        if (*p != '\0')
            *p++ = '\0';
    }
    return count;
}

static status_code_t parse_student(char *line, Student *out)
{
    if (!line || !out)
        return STATUS_ERROR_NULL_POINTER;

    char *tokens[FIELDS_COUNT];
    if (split_tokens(line, tokens, FIELDS_COUNT) != FIELDS_COUNT)
        return STATUS_ERROR_INVALID_FORMAT;

    unsigned long long id = 0;
    if (!parse_unsigned(tokens[0], UINT_MAX, &id) || !is_latin_word(tokens[1]) || !is_latin_word(tokens[2]))
        return STATUS_ERROR_INVALID_FORMAT;

    unsigned char grades[EXAMS_COUNT];
    for (size_t i = 0; i < EXAMS_COUNT; ++i)
    {
        unsigned long long grade = 0;
        if (!parse_unsigned(tokens[4 + i], MAX_GRADE, &grade) || grade < MIN_GRADE)
            return STATUS_ERROR_INVALID_FORMAT;
        grades[i] = (unsigned char)grade;
    }

    memset(out, 0, sizeof(*out));
    out->id = (unsigned int)id;
    out->name = string_duplicate(tokens[1]);
    out->surname = string_duplicate(tokens[2]);
    out->group = string_duplicate(tokens[3]);
    out->grades = (unsigned char *)malloc(EXAMS_COUNT * sizeof(unsigned char));
    if (!out->name || !out->surname || !out->group || !out->grades)
    {
        student_destroy(out);
        return STATUS_ERROR_OUT_OF_MEMORY;
    }
    memcpy(out->grades, grades, EXAMS_COUNT);
    return STATUS_OK;
}

static bool has_id(const Student *students, const size_t count, const unsigned int id)
{
    for (size_t i = 0; i < count; ++i)
    {
        if (students[i].id == id)
            return true;
    }
    return false;
}

static status_code_t load_students(const char *path, Student **out_students, size_t *out_count, size_t *out_bad_line)
{
    if (!path || !out_students || !out_count || !out_bad_line)
        return STATUS_ERROR_NULL_POINTER;

    FILE *file = fopen(path, "r");
    if (!file)
        return STATUS_ERROR_FILE_OPEN;

    Student *students = NULL;
    size_t count = 0;
    size_t capacity = 0;
    size_t line_number = 0;
    status_code_t sc = STATUS_OK;
    char line[LINE_SIZE];

    while (sc == STATUS_OK && fgets(line, sizeof(line), file))
    {
        ++line_number;
        if (!strchr(line, '\n') && !feof(file))
        {
            sc = STATUS_ERROR_INVALID_FORMAT;
            break;
        }
        if (strspn(line, " \t\r\n") == strlen(line))
            continue;

        if (count == capacity)
        {
            const size_t new_capacity = capacity == 0 ? 8 : capacity * 2;
            Student *new_students = (Student *)realloc(students, new_capacity * sizeof(Student));
            if (!new_students)
            {
                sc = STATUS_ERROR_OUT_OF_MEMORY;
                break;
            }
            students = new_students;
            capacity = new_capacity;
        }

        sc = parse_student(line, &students[count]);
        if (sc == STATUS_OK && has_id(students, count, students[count].id))
        {
            student_destroy(&students[count]);
            sc = STATUS_ERROR_DUPLICATE_ID;
        }
        if (sc == STATUS_OK)
            ++count;
    }

    if (sc == STATUS_OK && ferror(file))
        sc = STATUS_ERROR_FILE_READ;
    fclose(file);

    if (sc != STATUS_OK)
    {
        students_free(students, count);
        *out_bad_line = line_number;
        return sc;
    }

    *out_students = students;
    *out_count = count;
    return STATUS_OK;
}

static double student_average(const Student *student)
{
    unsigned int sum = 0;
    for (size_t i = 0; i < EXAMS_COUNT; ++i)
    {
        sum += student->grades[i];
    }
    return (double)sum / EXAMS_COUNT;
}

static status_code_t overall_average(const Student *students, const size_t count, double *out)
{
    if (!out || (!students && count > 0))
        return STATUS_ERROR_NULL_POINTER;
    if (count == 0)
        return STATUS_ERROR_EMPTY_COLLECTION;

    double sum = 0.0;
    for (size_t i = 0; i < count; ++i)
    {
        sum += student_average(&students[i]);
    }
    *out = sum / (double)count;
    return STATUS_OK;
}

static bool match_id(const Student *s, const void *key)
{
    return s->id == *(const unsigned int *)key;
}

static bool match_surname(const Student *s, const void *key)
{
    return strcmp(s->surname, (const char *)key) == 0;
}

static bool match_name(const Student *s, const void *key)
{
    return strcmp(s->name, (const char *)key) == 0;
}

static bool match_group(const Student *s, const void *key)
{
    return strcmp(s->group, (const char *)key) == 0;
}

static int compare_by_id(const void *a, const void *b)
{
    const unsigned int ia = ((const Student *)a)->id;
    const unsigned int ib = ((const Student *)b)->id;
    return (ia > ib) - (ia < ib);
}

static int compare_by_surname(const void *a, const void *b)
{
    return strcmp(((const Student *)a)->surname, ((const Student *)b)->surname);
}

static int compare_by_name(const void *a, const void *b)
{
    return strcmp(((const Student *)a)->name, ((const Student *)b)->name);
}

static int compare_by_group(const void *a, const void *b)
{
    return strcmp(((const Student *)a)->group, ((const Student *)b)->group);
}

static status_code_t find_students(const Student *students, const size_t count, const student_match_t match,
                                   const void *key, const Student ***out_found, size_t *out_found_count)
{
    if (!match || !key || !out_found || !out_found_count || (!students && count > 0))
        return STATUS_ERROR_NULL_POINTER;

    *out_found = NULL;
    *out_found_count = 0;
    if (count == 0)
        return STATUS_OK;

    const Student **found = (const Student **)malloc(count * sizeof(const Student *));
    if (!found)
        return STATUS_ERROR_OUT_OF_MEMORY;

    size_t found_count = 0;
    for (size_t i = 0; i < count; ++i)
    {
        if (match(&students[i], key))
            found[found_count++] = &students[i];
    }

    *out_found = found;
    *out_found_count = found_count;
    return STATUS_OK;
}

static status_code_t sort_students(Student *students, const size_t count, const student_compare_t compare)
{
    if (!compare || (!students && count > 0))
        return STATUS_ERROR_NULL_POINTER;
    if (count > 1)
        qsort(students, count, sizeof(Student), compare);
    return STATUS_OK;
}

static void print_student(FILE *stream, const Student *student)
{
    fprintf(stream, "%-10u %-15s %-15s %-10s", student->id, student->surname, student->name, student->group);
    for (size_t i = 0; i < EXAMS_COUNT; ++i)
    {
        fprintf(stream, " %3u", (unsigned int)student->grades[i]);
    }
    fprintf(stream, "   %.2f\n", student_average(student));
}

static void print_header(FILE *stream)
{
    fprintf(stream, "%-10s %-15s %-15s %-10s %-19s   %s\n", "ID", "Surname", "Name", "Group", "Grades", "Avg");
}

static void print_found(const Student *const *found, const size_t count)
{
    if (count == 0)
    {
        printf("Студенты не найдены.\n");
        return;
    }
    print_header(stdout);
    for (size_t i = 0; i < count; ++i)
    {
        print_student(stdout, found[i]);
    }
}

static void print_all(const Student *students, const size_t count)
{
    if (count == 0)
    {
        printf("Список студентов пуст.\n");
        return;
    }
    print_header(stdout);
    for (size_t i = 0; i < count; ++i)
    {
        print_student(stdout, &students[i]);
    }
}

static status_code_t trace_student_by_id(FILE *trace, const Student *students, const size_t count, const unsigned int id)
{
    if (!trace || (!students && count > 0))
        return STATUS_ERROR_NULL_POINTER;

    for (size_t i = 0; i < count; ++i)
    {
        if (students[i].id == id)
        {
            fprintf(trace, "Студент %u: %s %s, группа %s, средний балл %.2f\n", students[i].id,
                    students[i].surname, students[i].name, students[i].group, student_average(&students[i]));
            fflush(trace);
            return STATUS_OK;
        }
    }
    return STATUS_ERROR_NOT_FOUND;
}

static status_code_t trace_above_average(FILE *trace, const Student *students, const size_t count, const double eps,
                                         size_t *out_written)
{
    if (!trace || !out_written || (!students && count > 0))
        return STATUS_ERROR_NULL_POINTER;
    if (eps < 0.0)
        return STATUS_ERROR_INVALID_ARGUMENT;

    double average = 0.0;
    const status_code_t sc = overall_average(students, count, &average);
    if (sc != STATUS_OK)
        return sc;

    size_t written = 0;
    fprintf(trace, "Студенты со средним баллом выше общего среднего (%.2f):\n", average);
    for (size_t i = 0; i < count; ++i)
    {
        if (student_average(&students[i]) - average > eps)
        {
            fprintf(trace, "  %s %s\n", students[i].surname, students[i].name);
            ++written;
        }
    }
    fflush(trace);
    *out_written = written;
    return STATUS_OK;
}

static status_code_t read_line(char *buf, const size_t size)
{
    if (!buf || size == 0)
        return STATUS_ERROR_NULL_POINTER;
    if (!fgets(buf, (int)size, stdin))
        return STATUS_ERROR_INPUT_END;

    char *newline = strchr(buf, '\n');
    if (newline)
    {
        *newline = '\0';
        if (newline > buf && newline[-1] == '\r')
            newline[-1] = '\0';
        return STATUS_OK;
    }
    if (feof(stdin))
        return STATUS_OK;

    int c;
    while ((c = getchar()) != '\n' && c != EOF)
    {
    }
    return STATUS_ERROR_INVALID_ARGUMENT;
}

static status_code_t prompt_id(unsigned int *out_id)
{
    char line[LINE_SIZE];
    while (true)
    {
        printf("Введите id: ");
        const status_code_t sc = read_line(line, sizeof(line));
        if (sc == STATUS_ERROR_INPUT_END)
            return sc;

        unsigned long long id = 0;
        if (sc == STATUS_OK && parse_unsigned(line, UINT_MAX, &id))
        {
            *out_id = (unsigned int)id;
            return STATUS_OK;
        }
        printf("Некорректный id, повторите.\n");
    }
}

static status_code_t prompt_word(const char *prompt, const bool latin_only, char *buf, const size_t size)
{
    while (true)
    {
        printf("%s", prompt);
        const status_code_t sc = read_line(buf, size);
        if (sc == STATUS_ERROR_INPUT_END)
            return sc;

        char *tokens[1];
        if (sc == STATUS_OK && split_tokens(buf, tokens, 1) == 1 && (!latin_only || is_latin_word(tokens[0])))
        {
            memmove(buf, tokens[0], strlen(tokens[0]) + 1);
            return STATUS_OK;
        }
        printf("Некорректный ввод, повторите.\n");
    }
}

static const char *status_to_string(const status_code_t sc)
{
    switch (sc)
    {
    case STATUS_OK:
        return "Успешно";
    case STATUS_ERROR_NULL_POINTER:
        return "Передан нулевой указатель";
    case STATUS_ERROR_OUT_OF_MEMORY:
        return "Недостаточно памяти";
    case STATUS_ERROR_INVALID_ARGUMENT:
        return "Неверный аргумент";
    case STATUS_ERROR_FILE_OPEN:
        return "Не удается открыть файл";
    case STATUS_ERROR_FILE_READ:
        return "Ошибка чтения файла";
    case STATUS_ERROR_INVALID_FORMAT:
        return "Некорректный формат записи о студенте";
    case STATUS_ERROR_DUPLICATE_ID:
        return "Повторяющийся id студента";
    case STATUS_ERROR_NOT_FOUND:
        return "Студент не найден";
    case STATUS_ERROR_EMPTY_COLLECTION:
        return "Список студентов пуст";
    case STATUS_ERROR_INPUT_END:
        return "Ввод завершён";
    default:
        return "Неизвестная ошибка";
    }
}

static void print_status(const status_code_t sc)
{
    if (sc == STATUS_OK)
        printf("Готово.\n");
    else
        printf("Ошибка: %s (код: %d)\n", status_to_string(sc), (int)sc);
}

static void print_menu(void)
{
    printf("\n 1. Поиск по id\n");
    printf(" 2. Поиск по фамилии\n");
    printf(" 3. Поиск по имени\n");
    printf(" 4. Поиск по группе\n");
    printf(" 5. Сортировка по id\n");
    printf(" 6. Сортировка по фамилии\n");
    printf(" 7. Сортировка по имени\n");
    printf(" 8. Сортировка по группе\n");
    printf(" 9. Показать всех студентов\n");
    printf("10. Записать в трассировочный файл студента по id\n");
    printf("11. Записать в трассировочный файл студентов с баллом выше среднего\n");
    printf(" 0. Выход\n");
    printf("> ");
}

static status_code_t handle_search(const Student *students, const size_t count, const unsigned int option)
{
    static const student_match_t matchers[] = {match_id, match_surname, match_name, match_group};
    static const char *const prompts[] = {"", "Введите фамилию: ", "Введите имя: ", "Введите группу: "};

    char text[LINE_SIZE];
    unsigned int id = 0;
    const void *key = text;
    status_code_t sc = STATUS_OK;

    if (option == 1)
    {
        sc = prompt_id(&id);
        key = &id;
    }
    else
    {
        sc = prompt_word(prompts[option - 1], option != 4, text, sizeof(text));
    }
    if (sc != STATUS_OK)
        return sc;

    const Student **found = NULL;
    size_t found_count = 0;
    sc = find_students(students, count, matchers[option - 1], key, &found, &found_count);
    if (sc != STATUS_OK)
        return sc;

    print_found(found, found_count);
    free((void *)found);
    return STATUS_OK;
}

static status_code_t handle_sort(Student *students, const size_t count, const unsigned int option)
{
    static const student_compare_t comparators[] = {compare_by_id, compare_by_surname, compare_by_name, compare_by_group};

    const status_code_t sc = sort_students(students, count, comparators[option - 5]);
    if (sc == STATUS_OK)
        print_all(students, count);
    return sc;
}

static status_code_t handle_trace_by_id(FILE *trace, const Student *students, const size_t count)
{
    unsigned int id = 0;
    status_code_t sc = prompt_id(&id);
    if (sc != STATUS_OK)
        return sc;

    sc = trace_student_by_id(trace, students, count, id);
    print_status(sc);
    return STATUS_OK;
}

static status_code_t handle_trace_above_average(FILE *trace, const Student *students, const size_t count)
{
    size_t written = 0;
    const status_code_t sc = trace_above_average(trace, students, count, AVERAGE_EPS, &written);
    if (sc != STATUS_OK)
        return sc;

    printf("Записано студентов: %zu\n", written);
    return STATUS_OK;
}

static status_code_t run_dialog(Student *students, const size_t count, FILE *trace)
{
    char line[LINE_SIZE];
    while (true)
    {
        print_menu();
        status_code_t sc = read_line(line, sizeof(line));
        if (sc == STATUS_ERROR_INPUT_END)
            return STATUS_OK;

        unsigned long long option = 0;
        if (sc != STATUS_OK || !parse_unsigned(line, 11, &option))
        {
            printf("Неизвестная команда.\n");
            continue;
        }

        switch (option)
        {
        case 0:
            return STATUS_OK;
        case 1:
        case 2:
        case 3:
        case 4:
            sc = handle_search(students, count, (unsigned int)option);
            break;
        case 5:
        case 6:
        case 7:
        case 8:
            sc = handle_sort(students, count, (unsigned int)option);
            break;
        case 9:
            print_all(students, count);
            sc = STATUS_OK;
            break;
        case 10:
            sc = handle_trace_by_id(trace, students, count);
            break;
        default:
            sc = handle_trace_above_average(trace, students, count);
            break;
        }

        if (sc == STATUS_ERROR_INPUT_END)
            return STATUS_OK;
        if (sc != STATUS_OK)
            print_status(sc);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Использование: %s <файл со студентами> <трассировочный файл>\n", argv[0]);
        return STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (strcmp(argv[1], argv[2]) == 0)
    {
        printf("Ошибка: входной и трассировочный файлы должны различаться\n");
        return STATUS_ERROR_INVALID_ARGUMENT;
    }

    Student *students = NULL;
    size_t count = 0;
    size_t bad_line = 0;
    status_code_t sc = load_students(argv[1], &students, &count, &bad_line);
    if (sc != STATUS_OK)
    {
        if (bad_line > 0)
            printf("Ошибка: %s (строка %zu, код: %d)\n", status_to_string(sc), bad_line, (int)sc);
        else
            print_status(sc);
        return sc;
    }

    FILE *trace = fopen(argv[2], "w");
    if (!trace)
    {
        students_free(students, count);
        print_status(STATUS_ERROR_FILE_OPEN);
        return STATUS_ERROR_FILE_OPEN;
    }

    printf("Загружено студентов: %zu\n", count);
    sc = run_dialog(students, count, trace);

    fclose(trace);
    students_free(students, count);
    return sc;
}