#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum
{
    STATUS_OK,
    STATUS_ERROR_NULL_POINTER,
    STATUS_ERROR_OUT_OF_MEMORY,
    STATUS_ERROR_INVALID_ARGUMENT,
    STATUS_ERROR_OVERFLOW,
    STATUS_ERROR_NOT_FOUND,
    STATUS_ERROR_ALREADY_EXISTS,
    STATUS_ERROR_INPUT_END
} status_code_t;

typedef struct
{
    char *data;
    int length;
} String;

static status_code_t string_create(String *out, const char *src)
{
    if (!out || !src)
        return STATUS_ERROR_NULL_POINTER;

    const size_t len = strlen(src);
    if (len > INT_MAX)
        return STATUS_ERROR_OVERFLOW;

    char *data = (char *)malloc(len + 1);
    if (!data)
        return STATUS_ERROR_OUT_OF_MEMORY;

    memcpy(data, src, len + 1);
    out->data = data;
    out->length = (int)len;
    return STATUS_OK;
}

static void string_destroy(String *str)
{
    if (!str)
        return;
    free(str->data);
    str->data = NULL;
    str->length = 0;
}

static status_code_t string_compare(const String *a, const String *b, int *out_result)
{
    if (!a || !b || !out_result || !a->data || !b->data)
        return STATUS_ERROR_NULL_POINTER;

    if (a->length != b->length)
    {
        *out_result = a->length < b->length ? -1 : 1;
        return STATUS_OK;
    }

    const int cmp = memcmp(a->data, b->data, (size_t)a->length);
    *out_result = (cmp > 0) - (cmp < 0);
    return STATUS_OK;
}

static status_code_t string_equals(const String *a, const String *b, bool *out_result)
{
    if (!a || !b || !out_result || !a->data || !b->data)
        return STATUS_ERROR_NULL_POINTER;

    *out_result = a->length == b->length && memcmp(a->data, b->data, (size_t)a->length) == 0;
    return STATUS_OK;
}

static status_code_t string_copy(String *dst, const String *src)
{
    if (!dst || !src || !src->data)
        return STATUS_ERROR_NULL_POINTER;
    if (dst == src)
        return STATUS_OK;

    char *data = (char *)realloc(dst->data, (size_t)src->length + 1);
    if (!data)
        return STATUS_ERROR_OUT_OF_MEMORY;

    memcpy(data, src->data, (size_t)src->length + 1);
    dst->data = data;
    dst->length = src->length;
    return STATUS_OK;
}

static status_code_t string_clone(const String *src, String **out)
{
    if (!src || !out || !src->data)
        return STATUS_ERROR_NULL_POINTER;

    String *copy = (String *)malloc(sizeof(String));
    if (!copy)
        return STATUS_ERROR_OUT_OF_MEMORY;

    const status_code_t sc = string_create(copy, src->data);
    if (sc != STATUS_OK)
    {
        free(copy);
        return sc;
    }

    *out = copy;
    return STATUS_OK;
}

static status_code_t string_concat(String *dst, const String *src)
{
    if (!dst || !src || !dst->data || !src->data)
        return STATUS_ERROR_NULL_POINTER;

    const int src_len = src->length;
    if (dst->length > INT_MAX - src_len)
        return STATUS_ERROR_OVERFLOW;

    const int new_len = dst->length + src_len;
    char *data = (char *)realloc(dst->data, (size_t)new_len + 1);
    if (!data)
        return STATUS_ERROR_OUT_OF_MEMORY;

    dst->data = data;
    memcpy(dst->data + dst->length, src->data, (size_t)src_len);
    dst->data[new_len] = '\0';
    dst->length = new_len;
    return STATUS_OK;
}

static void string_delete(String *str)
{
    string_destroy(str);
    free(str);
}

#define LINE_SIZE 256
#define INDEX_LENGTH 6
#define ID_LENGTH 14
#define DATETIME_LENGTH 19
#define WEIGHT_EPS 1e-9

typedef struct
{
    String city;
    String street;
    unsigned int house;
    String building;
    unsigned int apartment;
    String index;
} Address;

typedef struct
{
    Address recipient;
    double weight;
    String id;
    String created;
    String delivery;
    bool is_delivered;
} Mail;

typedef struct
{
    Address *office;
    Mail *mails;
    size_t count;
    size_t capacity;
} Post;

typedef bool (*validator_t)(const char *);

static void address_destroy(Address *address)
{
    if (!address)
        return;
    string_destroy(&address->city);
    string_destroy(&address->street);
    string_destroy(&address->building);
    string_destroy(&address->index);
}

static void mail_destroy(Mail *mail)
{
    if (!mail)
        return;
    address_destroy(&mail->recipient);
    string_destroy(&mail->id);
    string_destroy(&mail->created);
    string_destroy(&mail->delivery);
}

static void post_destroy(Post *post)
{
    if (!post)
        return;
    for (size_t i = 0; i < post->count; ++i)
    {
        mail_destroy(&post->mails[i]);
    }
    free(post->mails);
    address_destroy(post->office);
    free(post->office);
    post->mails = NULL;
    post->office = NULL;
    post->count = 0;
    post->capacity = 0;
}

static bool is_digits(const char *s, const size_t length)
{
    if (!s || strlen(s) != length)
        return false;
    for (size_t i = 0; i < length; ++i)
    {
        if (!isdigit((unsigned char)s[i]))
            return false;
    }
    return true;
}

static bool parse_natural(const char *s, unsigned int *out)
{
    if (!s || !out || *s == '\0')
        return false;

    unsigned long long value = 0;
    for (; *s != '\0'; ++s)
    {
        if (!isdigit((unsigned char)*s))
            return false;
        value = value * 10 + (unsigned long long)(*s - '0');
        if (value > UINT_MAX)
            return false;
    }
    if (value == 0)
        return false;

    *out = (unsigned int)value;
    return true;
}

static bool parse_weight(const char *s, const double eps, double *out)
{
    if (!s || !out || *s == '\0' || eps < 0.0)
        return false;

    char *end = NULL;
    double value = strtod(s, &end);
    if (end == s || *end != '\0' || value != value || value > DBL_MAX || value < -DBL_MAX)
        return false;
    if (value < -eps)
        return false;
    if (value < 0.0)
        value = 0.0;

    *out = value;
    return true;
}

static int two_digits(const char *s)
{
    return (s[0] - '0') * 10 + (s[1] - '0');
}

static int days_in_month(const int month, const int year)
{
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    return month == 2 && leap ? 29 : days[month - 1];
}

static long long make_key(const int year, const int month, const int day,
                          const int hour, const int minute, const int second)
{
    return ((((year * 100LL + month) * 100 + day) * 100 + hour) * 100 + minute) * 100 + second;
}

static bool parse_datetime(const char *s, long long *out_key)
{
    if (!s || !out_key || strlen(s) != DATETIME_LENGTH)
        return false;

    for (size_t i = 0; i < DATETIME_LENGTH; ++i)
    {
        if (i == 2 || i == 5 || i == 13 || i == 16)
        {
            if (s[i] != ':')
                return false;
        }
        else if (i == 10)
        {
            if (s[i] != ' ')
                return false;
        }
        else if (!isdigit((unsigned char)s[i]))
        {
            return false;
        }
    }

    const int day = two_digits(s);
    const int month = two_digits(s + 3);
    const int year = two_digits(s + 6) * 100 + two_digits(s + 8);
    const int hour = two_digits(s + 11);
    const int minute = two_digits(s + 14);
    const int second = two_digits(s + 17);

    if (year < 1 || month < 1 || month > 12 || day < 1 || day > days_in_month(month, year) ||
        hour > 23 || minute > 59 || second > 59)
        return false;

    *out_key = make_key(year, month, day, hour, minute, second);
    return true;
}

static long long datetime_key(const String *str)
{
    long long key = 0;
    parse_datetime(str->data, &key);
    return key;
}

static status_code_t current_time_key(long long *out_key)
{
    if (!out_key)
        return STATUS_ERROR_NULL_POINTER;

    const time_t now = time(NULL);
    if (now == (time_t)-1)
        return STATUS_ERROR_INVALID_ARGUMENT;

    const struct tm *tm = localtime(&now);
    if (!tm)
        return STATUS_ERROR_INVALID_ARGUMENT;

    *out_key = make_key(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    return STATUS_OK;
}

static bool validate_any(const char *s)
{
    return s != NULL;
}

static bool validate_nonempty(const char *s)
{
    return s && *s != '\0';
}

static bool validate_natural(const char *s)
{
    unsigned int value = 0;
    return parse_natural(s, &value);
}

static bool validate_index(const char *s)
{
    return is_digits(s, INDEX_LENGTH);
}

static bool validate_id(const char *s)
{
    return is_digits(s, ID_LENGTH);
}

static bool validate_weight(const char *s)
{
    double value = 0.0;
    return parse_weight(s, WEIGHT_EPS, &value);
}

static bool validate_datetime(const char *s)
{
    long long key = 0;
    return parse_datetime(s, &key);
}

static int mail_compare(const Mail *a, const Mail *b)
{
    int result = 0;
    string_compare(&a->recipient.index, &b->recipient.index, &result);
    if (result == 0)
        string_compare(&a->id, &b->id, &result);
    return result;
}

static int compare_by_created(const void *a, const void *b)
{
    const long long ka = datetime_key(&(*(const Mail *const *)a)->created);
    const long long kb = datetime_key(&(*(const Mail *const *)b)->created);
    return (ka > kb) - (ka < kb);
}

static status_code_t post_find(const Post *post, const String *id, size_t *out_pos)
{
    if (!post || !id || !out_pos)
        return STATUS_ERROR_NULL_POINTER;

    for (size_t i = 0; i < post->count; ++i)
    {
        bool equal = false;
        if (string_equals(&post->mails[i].id, id, &equal) == STATUS_OK && equal)
        {
            *out_pos = i;
            return STATUS_OK;
        }
    }
    return STATUS_ERROR_NOT_FOUND;
}

static status_code_t post_add(Post *post, const Mail *mail)
{
    if (!post || !mail)
        return STATUS_ERROR_NULL_POINTER;

    size_t pos = 0;
    if (post_find(post, &mail->id, &pos) == STATUS_OK)
        return STATUS_ERROR_ALREADY_EXISTS;

    if (post->count == post->capacity)
    {
        const size_t new_capacity = post->capacity == 0 ? 4 : post->capacity * 2;
        if (new_capacity > SIZE_MAX / sizeof(Mail))
            return STATUS_ERROR_OVERFLOW;

        Mail *new_mails = (Mail *)realloc(post->mails, new_capacity * sizeof(Mail));
        if (!new_mails)
            return STATUS_ERROR_OUT_OF_MEMORY;

        post->mails = new_mails;
        post->capacity = new_capacity;
    }

    pos = 0;
    while (pos < post->count && mail_compare(&post->mails[pos], mail) < 0)
    {
        ++pos;
    }

    memmove(&post->mails[pos + 1], &post->mails[pos], (post->count - pos) * sizeof(Mail));
    post->mails[pos] = *mail;
    post->count++;
    return STATUS_OK;
}

static status_code_t post_remove(Post *post, const String *id)
{
    size_t pos = 0;
    const status_code_t sc = post_find(post, id, &pos);
    if (sc != STATUS_OK)
        return sc;

    mail_destroy(&post->mails[pos]);
    memmove(&post->mails[pos], &post->mails[pos + 1], (post->count - pos - 1) * sizeof(Mail));
    post->count--;
    return STATUS_OK;
}

static status_code_t post_mark_delivered(Post *post, const String *id)
{
    size_t pos = 0;
    const status_code_t sc = post_find(post, id, &pos);
    if (sc != STATUS_OK)
        return sc;

    post->mails[pos].is_delivered = true;
    return STATUS_OK;
}

static status_code_t post_collect(const Post *post, const bool delivered, const long long now_key,
                                  const Mail ***out_list, size_t *out_count)
{
    if (!post || !out_list || !out_count)
        return STATUS_ERROR_NULL_POINTER;

    *out_list = NULL;
    *out_count = 0;
    if (post->count == 0)
        return STATUS_OK;

    const Mail **list = (const Mail **)malloc(post->count * sizeof(const Mail *));
    if (!list)
        return STATUS_ERROR_OUT_OF_MEMORY;

    size_t count = 0;
    for (size_t i = 0; i < post->count; ++i)
    {
        const Mail *mail = &post->mails[i];
        const bool match = delivered
                               ? mail->is_delivered
                               : !mail->is_delivered && datetime_key(&mail->delivery) < now_key;
        if (match)
            list[count++] = mail;
    }

    qsort(list, count, sizeof(const Mail *), compare_by_created);
    *out_list = list;
    *out_count = count;
    return STATUS_OK;
}

static status_code_t read_line(char *buf, const size_t size)
{
    if (!buf || size == 0)
        return STATUS_ERROR_NULL_POINTER;
    if (!fgets(buf, (int)size, stdin))
        return STATUS_ERROR_INPUT_END;

    const size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
    {
        buf[len - 1] = '\0';
        if (len > 1 && buf[len - 2] == '\r')
            buf[len - 2] = '\0';
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

static status_code_t prompt_line(const char *prompt, const validator_t validate, String *out)
{
    char line[LINE_SIZE];
    while (true)
    {
        printf("%s", prompt);
        const status_code_t sc = read_line(line, sizeof(line));
        if (sc == STATUS_ERROR_INPUT_END)
            return sc;
        if (sc == STATUS_OK && validate(line))
            return string_create(out, line);
        printf("Некорректный ввод, повторите.\n");
    }
}

static status_code_t read_address(Address *out)
{
    memset(out, 0, sizeof(*out));
    String house = {NULL, 0};
    String apartment = {NULL, 0};

    status_code_t sc = prompt_line("  Город: ", validate_nonempty, &out->city);
    if (sc == STATUS_OK)
        sc = prompt_line("  Улица: ", validate_nonempty, &out->street);
    if (sc == STATUS_OK)
        sc = prompt_line("  Номер дома: ", validate_natural, &house);
    if (sc == STATUS_OK)
        sc = prompt_line("  Корпус (можно оставить пустым): ", validate_any, &out->building);
    if (sc == STATUS_OK)
        sc = prompt_line("  Номер квартиры: ", validate_natural, &apartment);
    if (sc == STATUS_OK)
        sc = prompt_line("  Индекс (6 цифр): ", validate_index, &out->index);

    if (sc == STATUS_OK)
    {
        parse_natural(house.data, &out->house);
        parse_natural(apartment.data, &out->apartment);
    }
    else
    {
        address_destroy(out);
    }
    string_destroy(&house);
    string_destroy(&apartment);
    return sc;
}

static status_code_t read_mail(Mail *out)
{
    memset(out, 0, sizeof(*out));
    String weight = {NULL, 0};

    printf("Адрес получателя:\n");
    status_code_t sc = read_address(&out->recipient);
    if (sc == STATUS_OK)
        sc = prompt_line("  Вес посылки: ", validate_weight, &weight);
    if (sc == STATUS_OK)
        sc = prompt_line("  Идентификатор (14 цифр): ", validate_id, &out->id);
    if (sc == STATUS_OK)
        sc = prompt_line("  Время создания (dd:MM:yyyy hh:mm:ss): ", validate_datetime, &out->created);

    while (sc == STATUS_OK)
    {
        sc = prompt_line("  Время вручения (dd:MM:yyyy hh:mm:ss): ", validate_datetime, &out->delivery);
        if (sc != STATUS_OK || datetime_key(&out->delivery) >= datetime_key(&out->created))
            break;
        printf("Время вручения не может быть раньше времени создания.\n");
        string_destroy(&out->delivery);
    }

    if (sc == STATUS_OK)
        parse_weight(weight.data, WEIGHT_EPS, &out->weight);
    else
        mail_destroy(out);
    string_destroy(&weight);
    return sc;
}

static void print_address(const Address *address)
{
    printf("г. %s, ул. %s, д. %u", address->city.data, address->street.data, address->house);
    if (address->building.length > 0)
        printf(", корп. %s", address->building.data);
    printf(", кв. %u, индекс %s\n", address->apartment, address->index.data);
}

static void print_mail(const Mail *mail)
{
    printf("  Идентификатор:  %s\n", mail->id.data);
    printf("  Получатель:     ");
    print_address(&mail->recipient);
    printf("  Вес:            %.3f\n", mail->weight);
    printf("  Создано:        %s\n", mail->created.data);
    printf("  Вручение:       %s\n", mail->delivery.data);
    printf("  Статус:         %s\n\n", mail->is_delivered ? "вручено" : "не вручено");
}

static void print_mail_list(const char *title, const Mail *const *list, const size_t count)
{
    printf("%s: %zu\n\n", title, count);
    for (size_t i = 0; i < count; ++i)
    {
        print_mail(list[i]);
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
    case STATUS_ERROR_OVERFLOW:
        return "Переполнение";
    case STATUS_ERROR_NOT_FOUND:
        return "Отправление не найдено";
    case STATUS_ERROR_ALREADY_EXISTS:
        return "Отправление с таким идентификатором уже существует";
    case STATUS_ERROR_INPUT_END:
        return "Ввод завершён";
    default:
        return "Неизвестная ошибка";
    }
}

static void print_string(const char *name, const String *str)
{
    printf("%s = \"%s\" (длина %d)\n", name, str->data, str->length);
}

static void print_compare(const String *a, const String *b)
{
    int order = 0;
    bool equal = false;
    if (string_compare(a, b, &order) != STATUS_OK || string_equals(a, b, &equal) != STATUS_OK)
        return;

    const char sign = order < 0 ? '<' : (order > 0 ? '>' : '=');
    printf("  \"%s\" %c \"%s\", равны: %s\n", a->data, sign, b->data, equal ? "да" : "нет");
}

static status_code_t run_demo(void)
{
    String a = {NULL, 0};
    String b = {NULL, 0};
    String c = {NULL, 0};
    String copy = {NULL, 0};
    String *clone = NULL;

    status_code_t sc = string_create(&a, "hello");
    if (sc == STATUS_OK)
        sc = string_create(&b, "world");
    if (sc == STATUS_OK)
        sc = string_create(&c, "hi");
    if (sc == STATUS_OK)
        sc = string_create(&copy, "");

    if (sc == STATUS_OK)
    {
        printf("Создание:\n");
        print_string("  a", &a);
        print_string("  b", &b);
        print_string("  c", &c);

        printf("\nСравнение (сначала по длине, затем лексикографически):\n");
        print_compare(&a, &b);
        print_compare(&b, &a);
        print_compare(&a, &c);
        print_compare(&a, &a);

        printf("\nКопирование a в существующую строку:\n");
        sc = string_copy(&copy, &a);
    }
    if (sc == STATUS_OK)
    {
        print_string("  copy", &copy);
        print_compare(&copy, &a);

        printf("\nКопирование b в новую строку в динамической памяти:\n");
        sc = string_clone(&b, &clone);
    }
    if (sc == STATUS_OK)
    {
        print_string("  clone", clone);

        printf("\nКонкатенация:\n");
        sc = string_concat(&a, &b);
    }
    if (sc == STATUS_OK)
    {
        print_string("  a + b", &a);
        sc = string_concat(&c, &c);
    }
    if (sc == STATUS_OK)
    {
        print_string("  c + c", &c);

        printf("\nОбработка ошибок:\n");
        printf("  string_create(NULL): %s\n", status_to_string(string_create(&copy, NULL)));
        printf("  string_concat(NULL): %s\n", status_to_string(string_concat(NULL, &a)));
    }

    string_destroy(&a);
    string_destroy(&b);
    string_destroy(&c);
    string_destroy(&copy);
    string_delete(clone);
    return sc;
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
    printf("\n1. Добавить отправление\n");
    printf("2. Удалить отправление\n");
    printf("3. Найти отправление по идентификатору\n");
    printf("4. Показать все отправления\n");
    printf("5. Отметить отправление как врученное\n");
    printf("6. Показать доставленные отправления\n");
    printf("7. Показать просроченные отправления\n");
    printf("8. Демонстрация работы String\n");
    printf("0. Выход\n");
    printf("> ");
}

static status_code_t handle_add(Post *post)
{
    Mail mail;
    status_code_t sc = read_mail(&mail);
    if (sc != STATUS_OK)
        return sc;

    sc = post_add(post, &mail);
    if (sc != STATUS_OK)
        mail_destroy(&mail);
    print_status(sc);
    return STATUS_OK;
}

static status_code_t handle_by_id(Post *post, const char action)
{
    String id = {NULL, 0};
    status_code_t sc = prompt_line("Идентификатор (14 цифр): ", validate_id, &id);
    if (sc != STATUS_OK)
        return sc;

    size_t pos = 0;
    switch (action)
    {
    case '2':
        sc = post_remove(post, &id);
        print_status(sc);
        break;
    case '3':
        sc = post_find(post, &id, &pos);
        if (sc == STATUS_OK)
            print_mail(&post->mails[pos]);
        else
            print_status(sc);
        break;
    default:
        sc = post_mark_delivered(post, &id);
        print_status(sc);
        break;
    }

    string_destroy(&id);
    return STATUS_OK;
}

static status_code_t handle_list(const Post *post, const bool delivered)
{
    long long now_key = 0;
    status_code_t sc = current_time_key(&now_key);
    if (sc != STATUS_OK)
        return sc;

    const Mail **list = NULL;
    size_t count = 0;
    sc = post_collect(post, delivered, now_key, &list, &count);
    if (sc != STATUS_OK)
        return sc;

    print_mail_list(delivered ? "Доставленные отправления" : "Просроченные отправления", list, count);
    free((void *)list);
    return STATUS_OK;
}

static void handle_show_all(const Post *post)
{
    printf("Почтовое отделение: ");
    print_address(post->office);
    printf("Всего отправлений: %zu\n\n", post->count);
    for (size_t i = 0; i < post->count; ++i)
    {
        print_mail(&post->mails[i]);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 1)
    {
        printf("Использование: %s\n", argv[0]);
        return STATUS_ERROR_INVALID_ARGUMENT;
    }

    Post post = {NULL, NULL, 0, 0};
    post.office = (Address *)malloc(sizeof(Address));
    if (!post.office)
    {
        print_status(STATUS_ERROR_OUT_OF_MEMORY);
        return STATUS_ERROR_OUT_OF_MEMORY;
    }

    printf("Адрес почтового отделения:\n");
    status_code_t sc = read_address(post.office);
    if (sc != STATUS_OK)
    {
        free(post.office);
        return sc == STATUS_ERROR_INPUT_END ? STATUS_OK : sc;
    }

    bool running = true;
    while (running)
    {
        print_menu();
        char line[LINE_SIZE];
        sc = read_line(line, sizeof(line));
        if (sc == STATUS_ERROR_INPUT_END)
            break;
        if (sc != STATUS_OK || strlen(line) != 1)
            line[0] = '?';

        switch (line[0])
        {
        case '1':
            sc = handle_add(&post);
            break;
        case '2':
        case '3':
        case '5':
            sc = handle_by_id(&post, line[0]);
            break;
        case '4':
            handle_show_all(&post);
            sc = STATUS_OK;
            break;
        case '6':
            sc = handle_list(&post, true);
            break;
        case '7':
            sc = handle_list(&post, false);
            break;
        case '8':
            sc = run_demo();
            break;
        case '0':
            running = false;
            sc = STATUS_OK;
            break;
        default:
            printf("Неизвестная команда.\n");
            sc = STATUS_OK;
            break;
        }

        if (sc == STATUS_ERROR_INPUT_END)
            running = false;
        else if (sc != STATUS_OK)
            print_status(sc);
    }

    post_destroy(&post);
    return STATUS_OK;
}