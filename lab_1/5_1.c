#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <float.h>
#define MAX_ITERATIONS 10000000

typedef enum {
    STATUS_OK,
    STATUS_ERROR_NULL_POINTER,
    STATUS_ERROR_INVALID_ARG_COUNT,
    STATUS_ERROR_INVALID_ARG_VALUE,
    STATUS_ERROR_INVALID_EPSILON,
    STATUS_ERROR_OUT_OF_RANGE,
    STATUS_ERROR_MATH_OVERFLOW
} status_code_t;

// An = An-1 * X/n
static status_code_t sum_a(const double x, const double epsilon, double *const out_val)
{
    if (out_val == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    if (epsilon <= 0.0)
    {
        return STATUS_ERROR_INVALID_EPSILON;
    }
    double sum = 0.0;
    double n_element = 1.0;
    size_t n = 0;
    while (fabs(n_element) >= epsilon)
    {
        sum += n_element;
        n++;
        if (n > MAX_ITERATIONS)
        {
            return STATUS_ERROR_MATH_OVERFLOW;
        }
        n_element *= (x / (double)n);
    }
    *out_val = sum;
    return STATUS_OK;
}

static status_code_t sum_b(const double x, const double epsilon, double *const out_val)
{
    if (out_val == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    if (epsilon <= 0.0)
    {
        return STATUS_ERROR_INVALID_EPSILON;
    }
    double sum = 0.0;
    double n_element = 1.0;
    size_t n = 0;
    while (fabs(n_element) >= epsilon)
    {
        sum += n_element;
        n++;
        if (n > MAX_ITERATIONS)
        {
            return STATUS_ERROR_MATH_OVERFLOW;
        }
        n_element *= -(x * x) / ((double)(2 * n) * (double)(2 * n - 1));
    }
    *out_val = sum;
    return STATUS_OK;
}

static status_code_t sum_c(const double x, const double epsilon, double *const out_val)
{
    if (out_val == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    if (epsilon <= 0.0)
    {
        return STATUS_ERROR_INVALID_EPSILON;
    }
    double sum = 0.0;
    double n_element = 1.0;
    size_t n = 0;

    // если |x| >= 1, ряд не сходится
    if (fabs(x) >= 1.0)
    {
        return STATUS_ERROR_OUT_OF_RANGE;
    }
    while (fabs(n_element) >= epsilon)
    {
        sum += n_element;
        n++;
        if (n > MAX_ITERATIONS)
        {
            return STATUS_ERROR_MATH_OVERFLOW;
        }
        n_element *= (9.0 * (double)(n * n) * x * x) / ((double)(3 * n - 2) * (double)(3 * n - 1));
    }
    *out_val = sum;
    return STATUS_OK;
}

static status_code_t sum_d(const double x, const double epsilon, double *const out_val)
{
    if (out_val == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    if (epsilon <= 0.0)
    {
        return STATUS_ERROR_INVALID_EPSILON;
    }

    // Ряд сходится при |x| < 1
    if (fabs(x) >= 1.0)
    {
        return STATUS_ERROR_OUT_OF_RANGE;
    }

    double sum = 0.0;
    // Начинается с n = 1
    double n_element = -(x * x) / 2.0;
    size_t n = 1;

    while (fabs(n_element) >= epsilon)
    {
        sum += n_element;
        n++;

        if (n > MAX_ITERATIONS)
        {
            return STATUS_ERROR_MATH_OVERFLOW;
        }

        n_element *= -((double)(2 * n - 1) * x * x) / (double)(2 * n);
    }

    *out_val = sum;
    return STATUS_OK;
}

static status_code_t parser_double_data_type(const char *str, double *result)
{
    if (str == NULL || result == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    if (*str == '\0')
    {
        return STATUS_ERROR_INVALID_ARG_VALUE;
    }
    while (*str == ' ' || *str == '\n')
    {
        str++;
    }
    char *endptr = NULL;
    double value = strtod(str, &endptr);
    if (*endptr != '\0' || endptr == str)
    {
        return STATUS_ERROR_INVALID_ARG_VALUE;
    }
    if (fabs(value) > DBL_MAX){
        return STATUS_ERROR_OUT_OF_RANGE;
    }
    *result = value;
    return STATUS_OK;
}

static void print_error(const status_code_t status)
{
    switch (status)
    {
    case STATUS_ERROR_INVALID_ARG_COUNT:
        fprintf(stderr, "Ошибка: Передано неверное количество аргументов.\n");
        fprintf(stderr, "Использование: ./5_1 <x> <epsilon>\n");
        break;
    case STATUS_ERROR_INVALID_ARG_VALUE:
        fprintf(stderr, "Ошибка: Введенные аргументы не являются числами.\n");
        break;
    case STATUS_ERROR_INVALID_EPSILON:
        fprintf(stderr, "Ошибка: Точность epsilon должна быть больше 0.\n");
        break;
    case STATUS_ERROR_OUT_OF_RANGE:
        fprintf(stderr, "Ошибка: Значение x выходит за область сходимости ряда.\n");
        break;
    case STATUS_ERROR_MATH_OVERFLOW:
        fprintf(stderr, "Ошибка: Ряд расходится или превышено число итераций.\n");
        break;
    default:
        fprintf(stderr, "Произошла неизвестная ошибка.\n");
        break;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        print_error(STATUS_ERROR_INVALID_ARG_COUNT);
        return STATUS_ERROR_INVALID_ARG_COUNT;
    }

    double x = 0.0;
    double epsilon = 0.0;
    status_code_t status = STATUS_OK;

    status = parser_double_data_type(argv[1], &x);
    if (status != STATUS_OK)
    {
        print_error(status);
        return status;
    }

    status = parser_double_data_type(argv[2], &epsilon);
    if (status != STATUS_OK)
    {
        print_error(status);
        return status;
    }

    double result_a = 0.0, result_b = 0.0, result_c = 0.0, result_d = 0.0;

    // Пункт a
    status = sum_a(x, epsilon, &result_a);
    if (status == STATUS_OK)
    {
        printf("a) Сумма = %.10f\n", result_a);
    }
    else
    {
        printf("a) ");
        print_error(status);
    }

    // Пункт b
    status = sum_b(x, epsilon, &result_b);
    if (status == STATUS_OK)
    {
        printf("b) Сумма = %.10f\n", result_b);
    }
    else
    {
        printf("b) ");
        print_error(status);
    }

    // Пункт c
    status = sum_c(x, epsilon, &result_c);
    if (status == STATUS_OK)
    {
        printf("c) Сумма = %.10f\n", result_c);
    }
    else
    {
        printf("c) ");
        print_error(status);
    }

    // Пункт d
    status = sum_d(x, epsilon, &result_d);
    if (status == STATUS_OK)
    {
        printf("d) Сумма = %.10f\n", result_d);
    }
    else
    {
        printf("d) ");
        print_error(status);
    }

    return STATUS_OK;
}