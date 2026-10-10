#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <float.h>

#define MAX_ITERATIONS 10000000

typedef enum {
    STATUS_OK,
    STATUS_ERROR_NULL_POINTER,
    STATUS_ERROR_OUT_OF_RANGE,
    STATUS_ERROR_INVALID_ARG_COUNT,
    STATUS_ERROR_INVALID_ARG_VALUE,
    STATUS_ERROR_INVALID_EPSILON,
    STATUS_ERROR_MATH_OVERFLOW
} status_code_t;

static double integrand_a(double x)
{
    if (x == 0.0)
    {
        return 1.0;
    }
    return log(1.0 + x) / x;
}

static double integrand_b(double x)
{
    return exp(-(x * x) / 2.0);
}

static double integrand_c(double x)
{
    if (x >= 1.0)
    {
        return 0.0;
    }
    return log(1.0 / (1.0 - x));
}

static double integrand_d(double x)
{
    if (x == 0.0)
    {
        return 1.0;
    }
    return pow(x, x);
}

static status_code_t calc_integral_rectangle(double (*func)(double), double a, double b, const double epsilon, double *const out_val)
{
    if (func == NULL || out_val == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    if (epsilon <= 0.0)
    {
        return STATUS_ERROR_INVALID_EPSILON;
    }

    size_t n = 10;
    double h = (b - a) / (double)n;
    
    double prev_integral = 0.0;
    for (size_t i = 0; i < n; i++)
    {
        double x = a + ((double)i + 0.5) * h;
        prev_integral += func(x);
    }
    prev_integral *= h;

    size_t iter = 0;
    while (iter < MAX_ITERATIONS)
    {
        n *= 2;
        h = (b - a) / (double)n;
        double curr_integral = 0.0;
        
        for (size_t i = 0; i < n; i++)
        {
            double x = a + ((double)i + 0.5) * h;
            curr_integral += func(x);
        }
        curr_integral *= h;

        if (fabs(curr_integral - prev_integral) < epsilon)
        {
            *out_val = curr_integral;
            return STATUS_OK;
        }

        prev_integral = curr_integral;
        iter++;
    }

    return STATUS_ERROR_MATH_OVERFLOW;
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
    if (fabs(value) > DBL_MAX)
    {
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
        printf("Ошибка: Передано неверное количество аргументов.\n");
        printf("Использование: ./app <epsilon>\n");
        break;
    case STATUS_ERROR_INVALID_ARG_VALUE:
        printf("Ошибка: Введенный аргумент не является числом.\n");
        break;
    case STATUS_ERROR_INVALID_EPSILON:
        printf("Ошибка: Точность epsilon должна быть больше 0.\n");
        break;
    case STATUS_ERROR_MATH_OVERFLOW:
        printf("Ошибка: Превышено максимальное число итераций или интеграл расходится.\n");
        break;
    case STATUS_ERROR_NULL_POINTER:
        printf("Ошибка: Попытка обращения к NULL-указателю.\n");
        break;
    case STATUS_ERROR_OUT_OF_RANGE:
        printf("Ошибка: Значение выходит за допустимый диапазон.\n");
        break;
    default:
        printf("Произошла неизвестная ошибка.\n");
        break;
    }
}

int main(int argc, char *argv[])
{   
    if (argc != 2)
    {
        print_error(STATUS_ERROR_INVALID_ARG_COUNT);
        return STATUS_ERROR_INVALID_ARG_COUNT;
    }

    double epsilon = 0.0;
    status_code_t status = parser_double_data_type(argv[1], &epsilon);
    if (status != STATUS_OK)
    {
        print_error(status);
        return status;
    }

    if (epsilon <= 0.0 || isnan(epsilon) || isinf(epsilon))
    {
        print_error(STATUS_ERROR_INVALID_EPSILON);
        return STATUS_ERROR_INVALID_EPSILON;
    }

    double result_a = 0.0, result_b = 0.0, result_c = 0.0, result_d = 0.0;

    // Пункт a
    status = calc_integral_rectangle(integrand_a, 0.0, 1.0, epsilon, &result_a);
    if (status == STATUS_OK)
    {
        printf("a) Интеграл = %.10f\n", result_a);
    }
    else
    {
        printf("a) ");
        print_error(status);
    }

    // Пункт b
    status = calc_integral_rectangle(integrand_b, 0.0, 1.0, epsilon, &result_b);
    if (status == STATUS_OK)
    {
        printf("b) Интеграл = %.10f\n", result_b);
    }
    else
    {
        printf("b) ");
        print_error(status);
    }

    // Пункт c
    status = calc_integral_rectangle(integrand_c, 0.0, 1.0, epsilon, &result_c);
    if (status == STATUS_OK)
    {
        printf("c) Интеграл = %.10f\n", result_c);
    }
    else
    {
        printf("c) ");
        print_error(status);
    }

    // Пункт d
    status = calc_integral_rectangle(integrand_d, 0.0, 1.0, epsilon, &result_d);
    if (status == STATUS_OK)
    {
        printf("d) Интеграл = %.10f\n", result_d);
    }
    else
    {
        printf("d) ");
        print_error(status);
    }

    return STATUS_OK;
}