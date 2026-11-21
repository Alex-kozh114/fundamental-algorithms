#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define MINUTES_IN_DAY (24 * 60)

typedef enum
{
    STATUS_OK,
    STATUS_ERROR_NULL_POINTER,
    STATUS_ERROR_OUT_OF_MEMORY,
    STATUS_ERROR_INVALID_INPUT,
    STATUS_ERROR_INVALID_TIME,
    STATUS_ERROR_DUPLICATE_ID,
    STATUS_ERROR_FILE_OPEN
} status_code_t;

typedef struct
{
    unsigned int id;
    int start;
    int finish;
} talk_t;

static int compare_by_finish(const void *a, const void *b)
{
    const talk_t *x = (const talk_t *)a;
    const talk_t *y = (const talk_t *)b;
    return x->finish - y->finish;
}

static status_code_t validate_talks(const talk_t *talks, const size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        if (talks[i].id == 0)
            return STATUS_ERROR_INVALID_INPUT;
        if (talks[i].start < 0 || talks[i].finish > MINUTES_IN_DAY || talks[i].start >= talks[i].finish)
            return STATUS_ERROR_INVALID_TIME;
        for (size_t j = i + 1; j < count; ++j)
        {
            if (talks[i].id == talks[j].id)
                return STATUS_ERROR_DUPLICATE_ID;
        }
    }
    return STATUS_OK;
}

static status_code_t max_talks(const talk_t *talks, const size_t count, size_t halls, size_t *result)
{
    if (!result || (!talks && count > 0))
        return STATUS_ERROR_NULL_POINTER;
    if (halls == 0)
        return STATUS_ERROR_INVALID_INPUT;

    status_code_t sc = validate_talks(talks, count);
    if (sc != STATUS_OK)
        return sc;

    *result = 0;
    if (count == 0)
        return STATUS_OK;
    if (halls > count)
        halls = count;

    talk_t *sorted = (talk_t *)malloc(count * sizeof(talk_t));
    int *hall_free_at = (int *)calloc(halls, sizeof(int));
    if (!sorted || !hall_free_at)
    {
        free(sorted);
        free(hall_free_at);
        return STATUS_ERROR_OUT_OF_MEMORY;
    }

    memcpy(sorted, talks, count * sizeof(talk_t));
    qsort(sorted, count, sizeof(talk_t), compare_by_finish);

    for (size_t i = 0; i < count; ++i)
    {
        int found = 0;
        size_t best = 0;
        for (size_t h = 0; h < halls; ++h)
        {
            if (hall_free_at[h] > sorted[i].start)
                continue;
            if (!found || hall_free_at[h] > hall_free_at[best])
            {
                best = h;
                found = 1;
            }
        }

        if (found)
        {
            hall_free_at[best] = sorted[i].finish;
            ++*result;
        }
    }

    free(sorted);
    free(hall_free_at);
    return STATUS_OK;
}

static status_code_t parse_time(const char *s, int *minutes)
{
    if (strlen(s) != 5 || s[2] != ':')
        return STATUS_ERROR_INVALID_TIME;
    for (int i = 0; i < 5; ++i)
    {
        if (i != 2 && !isdigit((unsigned char)s[i]))
            return STATUS_ERROR_INVALID_TIME;
    }

    const int h = (s[0] - '0') * 10 + (s[1] - '0');
    const int m = (s[3] - '0') * 10 + (s[4] - '0');
    if (h > 23 || m > 59)
        return STATUS_ERROR_INVALID_TIME;

    *minutes = h * 60 + m;
    return STATUS_OK;
}

static status_code_t read_talk(FILE *file, talk_t *talk)
{
    long long id = 0;
    char start[16];
    char finish[16];
    if (fscanf(file, "%lld %15s %15s", &id, start, finish) != 3)
        return STATUS_ERROR_INVALID_INPUT;
    if (id <= 0 || id > UINT_MAX)
        return STATUS_ERROR_INVALID_INPUT;

    status_code_t sc = parse_time(start, &talk->start);
    if (sc == STATUS_OK)
        sc = parse_time(finish, &talk->finish);
    if (sc != STATUS_OK)
        return sc;
    if (talk->start >= talk->finish)
        return STATUS_ERROR_INVALID_TIME;

    talk->id = (unsigned int)id;
    return STATUS_OK;
}

static status_code_t read_input(FILE *file, talk_t **talks, size_t *count, size_t *halls)
{
    long long n = 0;
    long long k = 0;
    if (fscanf(file, "%lld %lld", &n, &k) != 2 || n <= 0 || k <= 0)
        return STATUS_ERROR_INVALID_INPUT;

    talk_t *arr = (talk_t *)malloc((size_t)n * sizeof(talk_t));
    if (!arr)
        return STATUS_ERROR_OUT_OF_MEMORY;

    for (long long i = 0; i < n; ++i)
    {
        status_code_t sc = read_talk(file, &arr[i]);
        if (sc != STATUS_OK)
        {
            free(arr);
            return sc;
        }
    }

    *talks = arr;
    *count = (size_t)n;
    *halls = (size_t)k;
    return STATUS_OK;
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
    case STATUS_ERROR_INVALID_INPUT:
        return "Некорректные входные данные";
    case STATUS_ERROR_INVALID_TIME:
        return "Некорректное время доклада";
    case STATUS_ERROR_DUPLICATE_ID:
        return "Идентификаторы докладов повторяются";
    case STATUS_ERROR_FILE_OPEN:
        return "Не удалось открыть файл";
    default:
        return "Неизвестная ошибка";
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Использование: %s <файл>\n", argv[0]);
        return STATUS_ERROR_INVALID_INPUT;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file)
    {
        printf("Ошибка: %s\n", status_to_string(STATUS_ERROR_FILE_OPEN));
        return STATUS_ERROR_FILE_OPEN;
    }

    talk_t *talks = NULL;
    size_t count = 0;
    size_t halls = 0;
    size_t result = 0;

    status_code_t sc = read_input(file, &talks, &count, &halls);
    fclose(file);
    if (sc == STATUS_OK)
        sc = max_talks(talks, count, halls, &result);
    free(talks);

    if (sc != STATUS_OK)
    {
        printf("Ошибка: %s\n", status_to_string(sc));
        return sc;
    }

    printf("%zu\n", result);
    return 0;
}
