#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <ctype.h>

#define LINE_SIZE 1024
#define NUMBER_SIZE 64
#define COMPARE_EPS 1e-9

typedef enum
{
    STATUS_OK,
    STATUS_ERROR_NULL_POINTER,
    STATUS_ERROR_OUT_OF_MEMORY,
    STATUS_ERROR_INVALID_ARGUMENT,
    STATUS_ERROR_FILE_OPEN,
    STATUS_ERROR_INVALID_FORMAT,
    STATUS_ERROR_NOT_FOUND
} status_code_t;

typedef struct stop
{
    double x;
    double y;
    long long arrival;
    long long departure;
    char kind;
    struct stop *next;
} stop_t;

typedef struct vehicle
{
    char *number;
    stop_t *stops;
    struct vehicle *next;
} vehicle_t;

typedef struct
{
    size_t routes;
    double path;
    double longest_route;
    double shortest_route;
    long long longest_stop;
    long long shortest_stop;
    long long idle;
} stats_t;

static void storage_free(vehicle_t *head)
{
    while (head)
    {
        vehicle_t *next = head->next;
        while (head->stops)
        {
            stop_t *next_stop = head->stops->next;
            free(head->stops);
            head->stops = next_stop;
        }
        free(head->number);
        free(head);
        head = next;
    }
}

static bool to_seconds(const int d, const int mo, const int y, const int h, const int mi, const int s, long long *out)
{
    static const int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (y < 1 || mo < 1 || mo > 12 || h < 0 || h > 23 || mi < 0 || mi > 59 || s < 0 || s > 59)
        return false;
    const bool leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
    if (d < 1 || d > (mo == 2 && leap ? 29 : days_in_month[mo - 1]))
        return false;

    const long long yy = mo <= 2 ? y - 1 : y;
    const long long days = 365 * yy + yy / 4 - yy / 100 + yy / 400 + (153 * (mo + (mo > 2 ? -3 : 9)) + 2) / 5 + d;
    *out = days * 86400 + h * 3600LL + mi * 60LL + s;
    return true;
}

static bool matches_mask(const char *s, const char *mask)
{
    for (; *mask; ++s, ++mask)
    {
        if (*mask == '9' ? !isdigit((unsigned char)*s) : *s != *mask)
            return false;
    }
    return *s == '\0';
}

static bool parse_datetime(const char *date, const char *time, long long *out)
{
    int d, mo, y, h, mi, s;
    if (!matches_mask(date, "99.99.9999") || !matches_mask(time, "99:99:99"))
        return false;
    if (sscanf(date, "%d.%d.%d", &d, &mo, &y) != 3 || sscanf(time, "%d:%d:%d", &h, &mi, &s) != 3)
        return false;
    return to_seconds(d, mo, y, h, mi, s, out);
}

static status_code_t parse_record(const char *line, char *number, stop_t *stop)
{
    char date1[12], time1[10], date2[12], time2[10];
    int consumed = 0;
    const int read = sscanf(line, "%63s %11s %9s %11s %9s %c %n", number,
                            date1, time1, date2, time2, &stop->kind, &consumed);
    if (read != 6 || line[consumed] != '\0')
        return STATUS_ERROR_INVALID_FORMAT;
    if (!parse_datetime(date1, time1, &stop->arrival) || !parse_datetime(date2, time2, &stop->departure))
        return STATUS_ERROR_INVALID_FORMAT;
    if (stop->arrival > stop->departure || !strchr("SIE", stop->kind))
        return STATUS_ERROR_INVALID_FORMAT;
    return STATUS_OK;
}

static status_code_t add_stop(vehicle_t **head, const char *number, const stop_t *data)
{
    vehicle_t *v = *head;
    while (v && strcmp(v->number, number) != 0)
        v = v->next;

    stop_t *stop = (stop_t *)malloc(sizeof(stop_t));
    if (!stop)
        return STATUS_ERROR_OUT_OF_MEMORY;
    *stop = *data;

    if (!v)
    {
        v = (vehicle_t *)malloc(sizeof(vehicle_t));
        char *copy = (char *)malloc(strlen(number) + 1);
        if (!v || !copy)
        {
            free(v);
            free(copy);
            free(stop);
            return STATUS_ERROR_OUT_OF_MEMORY;
        }
        strcpy(copy, number);
        v->number = copy;
        v->stops = NULL;
        v->next = *head;
        *head = v;
    }

    stop_t **cursor = &v->stops;
    while (*cursor && (*cursor)->arrival <= stop->arrival)
        cursor = &(*cursor)->next;
    stop->next = *cursor;
    *cursor = stop;
    return STATUS_OK;
}

static status_code_t load_file(const char *path, vehicle_t **head, size_t *out_line)
{
    if (!path || !head || !out_line)
        return STATUS_ERROR_NULL_POINTER;

    FILE *file = fopen(path, "r");
    if (!file)
        return STATUS_ERROR_FILE_OPEN;

    char line[LINE_SIZE];
    char number[NUMBER_SIZE];
    stop_t stop;
    int consumed = 0;
    status_code_t sc = STATUS_OK;

    *out_line = 1;
    if (!fgets(line, sizeof(line), file) || sscanf(line, "%lf %lf %n", &stop.x, &stop.y, &consumed) != 2 ||
        line[consumed] != '\0' || !isfinite(stop.x) || !isfinite(stop.y))
        sc = STATUS_ERROR_INVALID_FORMAT;

    while (sc == STATUS_OK && fgets(line, sizeof(line), file))
    {
        ++*out_line;
        if (strspn(line, " \t\r\n") == strlen(line))
            continue;
        sc = parse_record(line, number, &stop);
        if (sc == STATUS_OK)
            sc = add_stop(head, number, &stop);
    }

    fclose(file);
    return sc;
}

static void compute_stats(const vehicle_t *v, stats_t *st)
{
    memset(st, 0, sizeof(*st));
    st->shortest_stop = -1;
    double route = 0.0;
    const stop_t *prev = NULL;

    for (const stop_t *s = v->stops; s; prev = s, s = s->next)
    {
        const long long duration = s->departure - s->arrival;
        st->idle += duration;
        if (duration > st->longest_stop)
            st->longest_stop = duration;
        if (st->shortest_stop < 0 || duration < st->shortest_stop)
            st->shortest_stop = duration;

        route = s->kind == 'S' || !prev ? 0.0 : route + hypot(s->x - prev->x, s->y - prev->y);
        if (s->kind == 'E')
        {
            if (st->routes == 0 || route > st->longest_route)
                st->longest_route = route;
            if (st->routes == 0 || route < st->shortest_route)
                st->shortest_route = route;
            st->path += route;
            st->routes++;
        }
    }
}

static double stat_value(const stats_t *st, const int option)
{
    switch (option)
    {
    case 1:
    case 2:
        return (double)st->routes;
    case 3:
    case 4:
        return st->path;
    case 5:
        return st->longest_route;
    case 6:
        return st->shortest_route;
    case 7:
        return (double)st->longest_stop;
    case 8:
        return (double)st->shortest_stop;
    default:
        return (double)st->idle;
    }
}

static status_code_t find_vehicle(const vehicle_t *head, const int option, const double eps,
                                  const vehicle_t **out, double *out_value)
{
    if (!out || !out_value)
        return STATUS_ERROR_NULL_POINTER;
    if (option < 1 || option > 9 || eps < 0.0)
        return STATUS_ERROR_INVALID_ARGUMENT;

    const bool find_max = option % 2 == 1;
    const vehicle_t *best = NULL;
    double best_value = 0.0;
    for (const vehicle_t *v = head; v; v = v->next)
    {
        stats_t st;
        compute_stats(v, &st);
        const double value = stat_value(&st, option);
        if (!best || (find_max ? value - best_value > eps : best_value - value > eps))
        {
            best = v;
            best_value = value;
        }
    }

    if (!best)
        return STATUS_ERROR_NOT_FOUND;
    *out = best;
    *out_value = best_value;
    return STATUS_OK;
}

static const char *status_to_string(const status_code_t sc)
{
    switch (sc)
    {
    case STATUS_ERROR_NULL_POINTER:
        return "Передан нулевой указатель";
    case STATUS_ERROR_OUT_OF_MEMORY:
        return "Недостаточно памяти";
    case STATUS_ERROR_INVALID_ARGUMENT:
        return "Неверный аргумент";
    case STATUS_ERROR_FILE_OPEN:
        return "Не удается открыть файл";
    case STATUS_ERROR_INVALID_FORMAT:
        return "Некорректный формат данных";
    case STATUS_ERROR_NOT_FOUND:
        return "Нет транспортных средств";
    default:
        return "Успешно";
    }
}

static void print_result(const vehicle_t *v, const int option, const double value)
{
    printf("ТС %s: ", v->number);
    if (option <= 2)
        printf("%.0f маршрутов\n", value);
    else if (option <= 6)
        printf("%.3f\n", value);
    else
        printf("%lld с\n", (long long)value);
}

static void run_dialog(const vehicle_t *head)
{
    char line[LINE_SIZE];
    while (true)
    {
        printf("\n1. Больше всех маршрутов\n2. Меньше всех маршрутов\n");
        printf("3. Самый длинный путь\n4. Самый короткий путь\n");
        printf("5. Самый длинный маршрут\n6. Самый короткий маршрут\n");
        printf("7. Самая долгая остановка\n8. Самая короткая остановка\n");
        printf("9. Самое большое время простоя\n0. Выход\n> ");

        int option = -1;
        int consumed = 0;
        if (!fgets(line, sizeof(line), stdin))
            return;
        if (sscanf(line, "%d %n", &option, &consumed) != 1 || line[consumed] != '\0' || option < 0 || option > 9)
        {
            printf("Неизвестная команда.\n");
            continue;
        }
        if (option == 0)
            return;

        const vehicle_t *v = NULL;
        double value = 0.0;
        const status_code_t sc = find_vehicle(head, option, COMPARE_EPS, &v, &value);
        if (sc == STATUS_OK)
            print_result(v, option, value);
        else
            printf("Ошибка: %s (код: %d)\n", status_to_string(sc), (int)sc);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Использование: %s <файл1> [<файл2> ...]\n", argv[0]);
        return STATUS_ERROR_INVALID_ARGUMENT;
    }

    vehicle_t *head = NULL;
    for (int i = 1; i < argc; ++i)
    {
        size_t line = 0;
        const status_code_t sc = load_file(argv[i], &head, &line);
        if (sc != STATUS_OK)
        {
            printf("Ошибка: %s (файл '%s'", status_to_string(sc), argv[i]);
            if (sc == STATUS_ERROR_INVALID_FORMAT)
                printf(", строка %zu", line);
            printf(", код: %d)\n", (int)sc);
            storage_free(head);
            return sc;
        }
    }

    run_dialog(head);
    storage_free(head);
    return STATUS_OK;
}