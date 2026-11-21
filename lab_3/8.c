#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>

#define NAME_SIZE 16
#define ARG_SIZE 1024
#define MAX_DEGREE 100000

typedef enum
{
    STATUS_OK,
    STATUS_ERROR_INVALID_ARGUMENT,
    STATUS_ERROR_OUT_OF_MEMORY,
    STATUS_ERROR_FILE_OPEN,
    STATUS_ERROR_SYNTAX,
    STATUS_ERROR_UNKNOWN_COMMAND,
    STATUS_ERROR_WRONG_ARGS,
    STATUS_ERROR_DIVISION_BY_ZERO,
    STATUS_ERROR_OVERFLOW
} status_code_t;

typedef struct Term
{
    long long coef;
    int degree;
    struct Term *next;
} Term;

typedef struct
{
    Term *head;
} Poly;

static void poly_free(Poly *p)
{
    if (p == NULL)
        return;
    while (p->head != NULL)
    {
        Term *next = p->head->next;
        free(p->head);
        p->head = next;
    }
}

static bool safe_add(const long long a, const long long b, long long *result)
{
    if (result == NULL)
        return false;
    if ((b > 0 && a > LLONG_MAX - b) || (b < 0 && a < LLONG_MIN - b))
        return false;
    *result = a + b;
    return true;
}

static bool safe_mult(const long long a, const long long b, long long *result)
{
    if (result == NULL)
        return false;
    if (a != 0 && b != 0)
    {
        if (a > 0 && b > 0 && a > LLONG_MAX / b)
            return false;
        if (a > 0 && b < 0 && b < LLONG_MIN / a)
            return false;
        if (a < 0 && b > 0 && a < LLONG_MIN / b)
            return false;
        if (a < 0 && b < 0 && a < LLONG_MAX / b)
            return false;
    }
    *result = a * b;
    return true;
}

static status_code_t add_term(Poly *p, const long long coef, const long long degree)
{
    if (p == NULL || degree < 0)
        return STATUS_ERROR_INVALID_ARGUMENT;
    if (coef == 0)
        return STATUS_OK;
    // LLONG_MIN запрещён, чтобы -coef всегда был представим
    if (degree > MAX_DEGREE || coef == LLONG_MIN)
        return STATUS_ERROR_OVERFLOW;

    Term *prev = NULL;
    Term *cur = p->head;
    while (cur != NULL && cur->degree > degree)
    {
        prev = cur;
        cur = cur->next;
    }

    if (cur != NULL && cur->degree == degree)
    {
        long long sum = 0;
        if (!safe_add(cur->coef, coef, &sum) || sum == LLONG_MIN)
            return STATUS_ERROR_OVERFLOW;
        cur->coef = sum;
        if (cur->coef == 0)
        {
            if (prev == NULL)
                p->head = cur->next;
            else
                prev->next = cur->next;
            free(cur);
        }
        return STATUS_OK;
    }

    Term *term = (Term *)malloc(sizeof(Term));
    if (term == NULL)
        return STATUS_ERROR_OUT_OF_MEMORY;
    term->coef = coef;
    term->degree = (int)degree;
    term->next = cur;
    if (prev == NULL)
        p->head = term;
    else
        prev->next = term;
    return STATUS_OK;
}

static status_code_t add_scaled(Poly *result, const Poly *p, const long long factor, const int shift)
{
    if (result == NULL || p == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;
    for (const Term *t = p->head; t != NULL; t = t->next)
    {
        long long coef = 0;
        if (!safe_mult(t->coef, factor, &coef))
            return STATUS_ERROR_OVERFLOW;
        const status_code_t sc = add_term(result, coef, (long long)t->degree + shift);
        if (sc != STATUS_OK)
            return sc;
    }
    return STATUS_OK;
}

static status_code_t poly_mult(const Poly *a, const Poly *b, Poly *result)
{
    if (a == NULL || b == NULL || result == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;
    for (const Term *t = a->head; t != NULL; t = t->next)
    {
        const status_code_t sc = add_scaled(result, b, t->coef, t->degree);
        if (sc != STATUS_OK)
            return sc;
    }
    return STATUS_OK;
}

static status_code_t poly_divmod(const Poly *a, const Poly *b, Poly *quotient, Poly *remainder)
{
    if (a == NULL || b == NULL || quotient == NULL || remainder == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;
    if (b->head == NULL)
        return STATUS_ERROR_DIVISION_BY_ZERO;
    status_code_t sc = add_scaled(remainder, a, 1, 0);

    while (sc == STATUS_OK && remainder->head != NULL && remainder->head->degree >= b->head->degree)
    {
        const long long coef = remainder->head->coef / b->head->coef;
        const int shift = remainder->head->degree - b->head->degree;
        if (coef == 0)
            break;
        sc = add_term(quotient, coef, shift);
        if (sc == STATUS_OK)
            sc = add_scaled(remainder, b, -coef, shift);
    }
    return sc;
}

static status_code_t poly_eval(const Poly *p, const long long x, long long *out)
{
    if (p == NULL || out == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;
    long long result = 0;
    for (const Term *t = p->head; t != NULL; t = t->next)
    {
        long long power = 1;
        long long value = 0;
        for (int i = 0; i < t->degree; i++)
        {
            if (!safe_mult(power, x, &power))
                return STATUS_ERROR_OVERFLOW;
        }
        if (!safe_mult(t->coef, power, &value) || !safe_add(result, value, &result))
            return STATUS_ERROR_OVERFLOW;
    }
    *out = result;
    return STATUS_OK;
}

static status_code_t poly_diff(const Poly *p, Poly *result)
{
    if (p == NULL || result == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;
    for (const Term *t = p->head; t != NULL && t->degree > 0; t = t->next)
    {
        long long coef = 0;
        if (!safe_mult(t->coef, t->degree, &coef))
            return STATUS_ERROR_OVERFLOW;
        const status_code_t sc = add_term(result, coef, t->degree - 1);
        if (sc != STATUS_OK)
            return sc;
    }
    return STATUS_OK;
}

static status_code_t poly_compose(const Poly *f, const Poly *g, Poly *result)
{
    if (f == NULL || g == NULL || result == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;
    const Term *t = f->head;
    for (int degree = t != NULL ? t->degree : -1; degree >= 0; degree--)
    {
        Poly product = {NULL};
        status_code_t sc = poly_mult(result, g, &product);
        poly_free(result);
        *result = product;
        if (sc == STATUS_OK && t != NULL && t->degree == degree)
        {
            sc = add_term(result, t->coef, 0);
            t = t->next;
        }
        if (sc != STATUS_OK)
            return sc;
    }
    return STATUS_OK;
}

static status_code_t read_number(const char **s, long long *out)
{
    if (s == NULL || *s == NULL || out == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;
    if (!isdigit((unsigned char)**s))
        return STATUS_ERROR_SYNTAX;

    *out = 0;
    while (isdigit((unsigned char)**s))
    {
        const int digit = **s - '0';
        if (*out > (LLONG_MAX - digit) / 10)
            return STATUS_ERROR_OVERFLOW;
        *out = *out * 10 + digit;
        (*s)++;
    }
    return STATUS_OK;
}

static status_code_t parse_integer(const char *s, long long *out)
{
    if (s == NULL || out == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;
    const bool negative = *s == '-';
    if (*s == '-' || *s == '+')
        s++;
    const status_code_t sc = read_number(&s, out);
    if (sc != STATUS_OK)
        return sc;
    if (*s != '\0')
        return STATUS_ERROR_SYNTAX;
    if (negative)
        *out = -*out;
    return STATUS_OK;
}

static status_code_t parse_poly(const char *s, Poly *out)
{
    if (s == NULL || out == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;
    const char *p = s;
    while (*p != '\0')
    {
        long long sign = 1;
        if (*p == '-')
            sign = -1;
        if (*p == '+' || *p == '-')
            p++;
        else if (p != s)
            return STATUS_ERROR_SYNTAX;

        long long coef = 1;
        long long degree = 0;
        const bool has_coef = isdigit((unsigned char)*p);
        status_code_t sc = has_coef ? read_number(&p, &coef) : STATUS_OK;

        if (sc == STATUS_OK && *p == 'x')
        {
            p++;
            degree = 1;
            if (*p == '^')
            {
                p++;
                sc = read_number(&p, &degree);
            }
        }
        else if (sc == STATUS_OK && !has_coef)
        {
            sc = STATUS_ERROR_SYNTAX;
        }

        if (sc == STATUS_OK)
            sc = add_term(out, sign * coef, degree);
        if (sc != STATUS_OK)
            return sc;
    }
    return p == s ? STATUS_ERROR_SYNTAX : STATUS_OK;
}

static void print_poly(const Poly *p)
{
    if (p == NULL)
        return;
    if (p->head == NULL)
        printf("0");

    for (const Term *t = p->head; t != NULL; t = t->next)
    {
        const long long coef = t->coef < 0 ? -t->coef : t->coef;
        if (t->coef < 0)
            printf(t == p->head ? "-" : " - ");
        else if (t != p->head)
            printf(" + ");

        if (coef != 1 || t->degree == 0)
            printf("%lld", coef);
        if (t->degree > 0)
            printf("x");
        if (t->degree > 1)
            printf("^%d", t->degree);
    }
    printf("\n");
}

static status_code_t read_clean_text(const char *path, char **out)
{
    if (path == NULL || out == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;
    FILE *file = fopen(path, "r");
    if (file == NULL)
        return STATUS_ERROR_FILE_OPEN;

    size_t size = 0;
    size_t capacity = 256;
    char *buf = (char *)malloc(capacity);
    status_code_t sc = buf == NULL ? STATUS_ERROR_OUT_OF_MEMORY : STATUS_OK;
    int c;

    while (sc == STATUS_OK && (c = fgetc(file)) != EOF)
    {
        if (c == '%')
        {
            while (c != EOF && c != '\n')
                c = fgetc(file);
            continue;
        }
        if (c == '[')
        {
            while ((c = fgetc(file)) != EOF && c != ']' && c != '[')
            {
            }
            if (c != ']')
                sc = STATUS_ERROR_SYNTAX;
            continue;
        }
        if (isspace(c))
            continue;

        if (size + 1 == capacity)
        {
            capacity *= 2;
            char *new_buf = (char *)realloc(buf, capacity);
            if (new_buf == NULL)
            {
                sc = STATUS_ERROR_OUT_OF_MEMORY;
                break;
            }
            buf = new_buf;
        }
        buf[size++] = (char)c;
    }
    fclose(file);

    if (sc != STATUS_OK)
    {
        free(buf);
        return sc;
    }
    buf[size] = '\0';
    *out = buf;
    return STATUS_OK;
}

// Выполняет команду. Многочлен-результат кладётся в сумматор acc,
// для Eval значение возвращается через value (is_value = true).
static status_code_t execute(const char *name, const char *arg1, const char *arg2, const int argc, Poly *acc,
                             bool *is_value, long long *value)
{
    if (name == NULL || arg1 == NULL || arg2 == NULL || acc == NULL || is_value == NULL || value == NULL)
        return STATUS_ERROR_INVALID_ARGUMENT;
    const bool is_eval = strcmp(name, "Eval") == 0;
    const bool is_diff = strcmp(name, "Diff") == 0;
    const bool is_binary = strcmp(name, "Add") == 0 || strcmp(name, "Sub") == 0 || strcmp(name, "Mult") == 0 ||
                           strcmp(name, "Div") == 0 || strcmp(name, "Mod") == 0 || strcmp(name, "Cmps") == 0;
    if (!is_eval && !is_diff && !is_binary)
        return STATUS_ERROR_UNKNOWN_COMMAND;

    const int full_argc = is_diff ? 1 : 2;
    if (argc != full_argc && argc != full_argc - 1)
        return STATUS_ERROR_WRONG_ARGS;

    Poly first = {NULL};
    Poly second = {NULL};
    Poly result = {NULL};
    Poly unused = {NULL};
    const Poly *a = argc == full_argc ? &first : acc;
    const char *last_arg = argc == 2 ? arg2 : arg1;
    status_code_t sc = argc == full_argc ? parse_poly(arg1, &first) : STATUS_OK;

    *is_value = is_eval;
    if (sc == STATUS_OK && is_eval)
    {
        long long x = 0;
        sc = parse_integer(last_arg, &x);
        if (sc == STATUS_OK)
            sc = poly_eval(a, x, value);
        poly_free(&first);
        return sc;
    }

    if (sc == STATUS_OK && is_binary)
        sc = parse_poly(last_arg, &second);

    if (sc == STATUS_OK)
    {
        if (strcmp(name, "Add") == 0 || strcmp(name, "Sub") == 0)
        {
            sc = add_scaled(&result, a, 1, 0);
            if (sc == STATUS_OK)
                sc = add_scaled(&result, &second, name[0] == 'A' ? 1 : -1, 0);
        }
        else if (strcmp(name, "Mult") == 0)
            sc = poly_mult(a, &second, &result);
        else if (strcmp(name, "Div") == 0)
            sc = poly_divmod(a, &second, &result, &unused);
        else if (strcmp(name, "Mod") == 0)
            sc = poly_divmod(a, &second, &unused, &result);
        else if (strcmp(name, "Cmps") == 0)
            sc = poly_compose(a, &second, &result);
        else
            sc = poly_diff(a, &result);
    }

    poly_free(&first);
    poly_free(&second);
    poly_free(&unused);
    if (sc != STATUS_OK)
    {
        poly_free(&result);
        return sc;
    }

    poly_free(acc);
    *acc = result;
    return STATUS_OK;
}

static const char *status_to_string(const status_code_t sc)
{
    switch (sc)
    {
    case STATUS_ERROR_INVALID_ARGUMENT:
        return "некорректный аргумент функции";
    case STATUS_ERROR_OUT_OF_MEMORY:
        return "недостаточно памяти";
    case STATUS_ERROR_FILE_OPEN:
        return "не удается открыть файл";
    case STATUS_ERROR_SYNTAX:
        return "синтаксическая ошибка";
    case STATUS_ERROR_UNKNOWN_COMMAND:
        return "неизвестная команда";
    case STATUS_ERROR_WRONG_ARGS:
        return "неверное количество аргументов";
    case STATUS_ERROR_DIVISION_BY_ZERO:
        return "деление на нулевой многочлен";
    case STATUS_ERROR_OVERFLOW:
        return "переполнение (слишком большой коэффициент или степень)";
    default:
        return "успешно";
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Использование: %s <файл с инструкциями>\n", argv[0]);
        return 1;
    }

    char *text = NULL;
    status_code_t sc = read_clean_text(argv[1], &text);
    if (sc != STATUS_OK)
    {
        printf("Ошибка: %s\n", status_to_string(sc));
        return sc;
    }

    Poly acc = {NULL};
    const char *p = text;
    while (*p != '\0')
    {
        char name[NAME_SIZE];
        char arg1[ARG_SIZE] = "";
        char arg2[ARG_SIZE] = "";
        int args = 0;
        int n = 0;

        if (sscanf(p, "%15[A-Za-z](%n", name, &n) != 1 || n == 0)
            n = 0;
        p += n;

        if (n > 0 && *p != ')' && sscanf(p, "%1023[^,)]%n", arg1, &n) == 1)
        {
            p += n;
            args = 1;
            if (*p == ',' && sscanf(p + 1, "%1023[^,)]%n", arg2, &n) == 1)
            {
                p += n + 1;
                args = 2;
            }
        }

        if (n == 0 || p[0] != ')' || p[1] != ';')
        {
            printf("Ошибка: %s\n", status_to_string(STATUS_ERROR_SYNTAX));
            p = strchr(p, ';');
            if (p == NULL)
                break;
            p++;
            continue;
        }
        p += 2;

        bool is_value = false;
        long long value = 0;
        sc = execute(name, arg1, arg2, args, &acc, &is_value, &value);
        if (sc != STATUS_OK)
        {
            printf("%s: ошибка - %s\n", name, status_to_string(sc));
        }
        else if (is_value)
        {
            printf("%s = %lld\n", name, value);
        }
        else
        {
            printf("%s = ", name);
            print_poly(&acc);
        }
    }

    poly_free(&acc);
    free(text);
    return 0;
}