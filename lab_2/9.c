#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>

#define MAX_ITERATIONS  64
#define MAX_DENOMINATOR 1000000000LL
#define EPSILON         1e-9

typedef enum {
    SUCCESS = 0,
    ERROR_NULL_POINTER,
    ERROR_INVALID_EPSILON,
    ERROR_INVALID_BASE,
    ERROR_INVALID_FRACTION,
    ERROR_INVALID_COUNT,
    ERROR_LIMIT_EXCEEDED
} status_code_t;

static long long gcd(long long a, long long b)
{
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b != 0) {
        long long t = b;
        b = a % b;
        a = t;
    }
    return a;
}

static status_code_t validate_epsilon(double epsilon)
{
    if (!isfinite(epsilon) || epsilon <= 0.0 || epsilon >= 1.0) {
        return ERROR_INVALID_EPSILON;
    }
    return SUCCESS;
}

static status_code_t find_denominator(double x, double epsilon, long long *out_q)
{
    if (out_q == NULL) {
        return ERROR_NULL_POINTER;
    }
    status_code_t st = validate_epsilon(epsilon);
    if (st != SUCCESS) {
        return st;
    }

    double val = x;
    long long p0 = 0, q0 = 1;
    long long p1 = 1, q1 = 0;

    for (int iter = 0; iter < MAX_ITERATIONS; ++iter) {
        double fl = floor(val);
        if (fl > (double)MAX_DENOMINATOR) {
            return ERROR_LIMIT_EXCEEDED;
        }
        long long a = (long long)fl;
        if (q1 != 0 && a > (MAX_DENOMINATOR - q0) / q1) {
            return ERROR_LIMIT_EXCEEDED;
        }

        long long p2 = a * p1 + p0;
        long long q2 = a * q1 + q0;
        if (q2 <= 0) {
            return ERROR_LIMIT_EXCEEDED;
        }

        if (fabs(x - (double)p2 / (double)q2) < epsilon) {
            *out_q = q2;
            return SUCCESS;
        }

        double diff = val - fl;
        if (diff <= 0.0) {
            return ERROR_LIMIT_EXCEEDED;
        }

        val = 1.0 / diff;
        p0 = p1;
        q0 = q1;
        p1 = p2;
        q1 = q2;
    }

    return ERROR_LIMIT_EXCEEDED;
}

static status_code_t is_finite_representation(double num, int base,
                                              double epsilon, bool *result)
{
    if (result == NULL) {
        return ERROR_NULL_POINTER;
    }
    status_code_t st = validate_epsilon(epsilon);
    if (st != SUCCESS) {
        return st;
    }
    if (base < 2) {
        return ERROR_INVALID_BASE;
    }
    if (!isfinite(num) || num <= 0.0 || num >= 1.0) {
        return ERROR_INVALID_FRACTION;
    }

    long long q = 0;
    st = find_denominator(num, epsilon, &q);
    if (st != SUCCESS) {
        return st;
    }

    long long b = (long long)base;
    long long g = gcd(q, b);
    while (g > 1) {
        while (q % g == 0) {
            q /= g;
        }
        g = gcd(q, b);
    }

    *result = (q == 1);
    return SUCCESS;
}

static status_code_t process_fractions(double epsilon, int base,
                                       bool *results, int count, ...)
{
    if (results == NULL) {
        return ERROR_NULL_POINTER;
    }
    status_code_t st = validate_epsilon(epsilon);
    if (st != SUCCESS) {
        return st;
    }
    if (base < 2) {
        return ERROR_INVALID_BASE;
    }
    if (count <= 0) {
        return ERROR_INVALID_COUNT;
    }

    va_list args;
    va_start(args, count);

    for (int i = 0; i < count; ++i) {
        double fraction = va_arg(args, double);
        st = is_finite_representation(fraction, base, epsilon, &results[i]);
        if (st != SUCCESS) {
            va_end(args);
            return st;
        }
    }

    va_end(args);
    return SUCCESS;
}

static const char *status_message(status_code_t st)
{
    switch (st) {
        case SUCCESS:                return "успешно";
        case ERROR_NULL_POINTER:     return "нулевой указатель";
        case ERROR_INVALID_EPSILON:  return "epsilon должен быть в (0;1)";
        case ERROR_INVALID_BASE:     return "основание должно быть >= 2";
        case ERROR_INVALID_FRACTION: return "число должно быть в (0;1)";
        case ERROR_INVALID_COUNT:    return "количество чисел должно быть > 0";
        case ERROR_LIMIT_EXCEEDED:   return "не удалось восстановить дробь (превышен лимит)";
        default:                     return "неизвестная ошибка";
    }
}

static void print_results(int base, const double *vals, const bool *res, int count)
{
    printf("Основание %d:\n", base);
    for (int i = 0; i < count; ++i) {
        printf("  %-20.17g -> %s\n", vals[i], res[i] ? "конечное" : "бесконечное");
    }
    printf("\n");
}

int main(void)
{
    bool res[8];
    status_code_t st;

    {
        const double v[] = {0.5, 0.25, 0.1, 0.3, 0.123456, 1.0 / 3.0};
        st = process_fractions(EPSILON, 10, res, 6, v[0], v[1], v[2], v[3], v[4], v[5]);
        if (st == SUCCESS) print_results(10, v, res, 6);
        else printf("Ошибка: %s\n\n", status_message(st));
    }
    {
        const double v[] = {0.5, 0.25, 0.125, 0.1, 0.75};
        st = process_fractions(EPSILON, 2, res, 5, v[0], v[1], v[2], v[3], v[4]);
        if (st == SUCCESS) print_results(2, v, res, 5);
        else printf("Ошибка: %s\n\n", status_message(st));
    }
    {
        const double v[] = {0.5, 1.0 / 3.0, 1.0 / 9.0, 2.0 / 3.0};
        st = process_fractions(EPSILON, 3, res, 4, v[0], v[1], v[2], v[3]);
        if (st == SUCCESS) print_results(3, v, res, 4);
        else printf("Ошибка: %s\n\n", status_message(st));
    }
    {
        const double v[] = {0.5, 0.25, 0.2, 1.0 / 3.0};
        st = process_fractions(EPSILON, 6, res, 4, v[0], v[1], v[2], v[3]);
        if (st == SUCCESS) print_results(6, v, res, 4);
        else printf("Ошибка: %s\n\n", status_message(st));
    }
    {
        const double v[] = {0.5, 0.0625, 0.1};
        st = process_fractions(EPSILON, 16, res, 3, v[0], v[1], v[2]);
        if (st == SUCCESS) print_results(16, v, res, 3);
        else printf("Ошибка: %s\n\n", status_message(st));
    }

    printf("--- проверка ошибок ---\n");

    st = process_fractions(EPSILON, 1, res, 1, 0.5);
    printf("основание 1:      %s\n", status_message(st));

    st = process_fractions(EPSILON, 10, res, 2, 0.5, 1.5);
    printf("число 1.5:        %s\n", status_message(st));

    st = process_fractions(EPSILON, 10, res, 0);
    printf("count = 0:        %s\n", status_message(st));

    st = process_fractions(0.0, 10, res, 1, 0.5);
    printf("epsilon = 0:      %s\n", status_message(st));

    st = process_fractions(EPSILON, 10, NULL, 1, 0.5);
    printf("results = NULL:   %s\n", status_message(st));

    return 0;
}
