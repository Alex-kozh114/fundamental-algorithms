#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <limits.h>
#include <float.h>

typedef enum {
    STATUS_OK,
    STATUS_ERROR_OUT_OF_RANGE,
    STATUS_ERROR_INVALID_ARG_COUNT,
    STATUS_ERROR_INVALID_OPTION,
    STATUS_ERROR_INVALID_ARG_VALUE,
    STATUS_ERROR_INVALID_EPSILON,
    STATUS_ERROR_DIVISION_BY_ZERO,
    STATUS_ERROR_NULL_POINTER,
    STATUS_ERROR_INVALID_FORMAT
} status_code_t;

static status_code_t parser_long_long_data_type(const char *str, long long *result)
{
    if (str == NULL || result == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }

    while (*str == ' ' || *str == '\n')
    {
        str++;
    }

    if (*str == '\0')
    {
        return STATUS_ERROR_INVALID_ARG_VALUE;
    }

    const char *p = str;
    bool negative = false;

    if (*p == '-')
    {
        negative = true;
        p++;
    }
    else if (*p == '+')
    {
        p++;
    }

    while (*p == '0')
    {
        p++;
    }

    size_t digits_count = 0;
    const char *start_digits = p;
    while (*p >= '0' && *p <= '9')
    {
        digits_count++;
        p++;
    }
    if (digits_count > 19)
    {
        return STATUS_ERROR_OUT_OF_RANGE;
    }
    if (digits_count == 0 && (p == str || *p != '\0'))
    {
        if (p > str && *(p - 1) == '0' && (*p == '\0' || *p == ' ' || *p == '\n'))
        {
            *result = 0;
            return STATUS_OK;
        }
        return STATUS_ERROR_INVALID_ARG_VALUE;
    }
    if (digits_count == 19)
    {
        if (!negative)
        {
            // llong_max 9223372036854775807
            if (strncmp(start_digits, "9223372036854775807", 19) > 0)
            {
                return STATUS_ERROR_OUT_OF_RANGE;
            }
        }
        else
        {
            // llong_min 9223372036854775808
            if (strncmp(start_digits, "9223372036854775808", 19) > 0)
            {
                return STATUS_ERROR_OUT_OF_RANGE;
            }
        }
    }
    char *endptr = NULL;
    long long value = strtoll(str, &endptr, 10);

    if (endptr == str)
    {
        return STATUS_ERROR_INVALID_ARG_VALUE;
    }

    if (*endptr != '\0')
    {
        return STATUS_ERROR_INVALID_FORMAT;
    }

    *result = value;
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

static status_code_t flag_m(const long long a, const long long b, bool *is_multiple)
{
    if (is_multiple == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    if (a == 0 || b == 0)
    {
        return STATUS_ERROR_DIVISION_BY_ZERO;
    }

    *is_multiple = (a % b == 0);
    return STATUS_OK;
}

static status_code_t flag_t(const double eps, const double a, const double b, const double c, bool *is_right_triangle)
{
    if (is_right_triangle == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    if (eps <= 0.0)
    {
        return STATUS_ERROR_INVALID_EPSILON;
    }
    if (a <= 0.0 || b <= 0.0 || c <= 0.0)
    {
        return STATUS_ERROR_INVALID_ARG_VALUE;
    }

    if (fabs(a * a + b * b - c * c) < eps ||
        fabs(a * a + c * c - b * b) < eps ||
        fabs(b * b + c * c - a * a) < eps)
    {
        *is_right_triangle = true;
    }
    else
    {
        *is_right_triangle = false;
    }

    return STATUS_OK;
}

static status_code_t solve_quadratic_equation(
    const double a, const double b, const double c, const double eps,
    double *x1, double *x2, int *count
) {
    if (x1 == NULL || x2 == NULL || count == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    if (eps <= 0.0)
    {
        return STATUS_ERROR_INVALID_EPSILON;
    }

    if (fabs(a) < eps)
    {
        if (fabs(b) < eps)
        {
            *count = (fabs(c) < eps) ? -1 : 0;
        }
        else
        {
            *x1 = -c / b;
            *count = 1;
        }
        return STATUS_OK;
    }

    double D = b * b - 4.0 * a * c;

    if (D < -eps)
    {
        *count = 0;
    }
    else if (fabs(D) < eps)
    {
        *x1 = -b / (2.0 * a);
        *count = 1;
    }
    else
    {
        double sq = sqrt(D);
        *x1 = (-b - sq) / (2.0 * a);
        *x2 = (-b + sq) / (2.0 * a);
        *count = 2;
    }

    return STATUS_OK;
}

static bool same_coef(double a1, double b1, double c1, double a2, double b2, double c2, double eps)
{
    return fabs(a1 - a2) < eps &&
           fabs(b1 - b2) < eps &&
           fabs(c1 - c2) < eps;
}

static status_code_t flag_q(const double eps, const double a, const double b, const double c)
{
    if (eps <= 0.0)
    {
        return STATUS_ERROR_INVALID_EPSILON;
    }

    double coeffs[3] = {a, b, c};
    double seen[6][3];
    int seen_count = 0;

    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            for (int k = 0; k < 3; ++k)
            {
                if (i == j || i == k || j == k) continue;

                double ca = coeffs[i], cb = coeffs[j], cc = coeffs[k];

                bool is_duplicate = false;
                for (int m = 0; m < seen_count; ++m)
                {
                    if (same_coef(ca, cb, cc, seen[m][0], seen[m][1], seen[m][2], eps))
                    {
                        is_duplicate = true;
                        break;
                    }
                }
                if (is_duplicate) continue;

                seen[seen_count][0] = ca;
                seen[seen_count][1] = cb;
                seen[seen_count][2] = cc;
                seen_count++;

                double x1 = 0, x2 = 0;
                int count = 0;
                status_code_t status = solve_quadratic_equation(ca, cb, cc, eps, &x1, &x2, &count);
                if (status != STATUS_OK) return status;

                printf("Уравнение (%.4lf)x^2 + (%.4lf)x + (%.4lf) = 0:\n  ", ca, cb, cc);
                switch (count)
                {
                    case 2:  printf("Корни: x1 = %.4lf, x2 = %.4lf\n", x1, x2); break;
                    case 1:  printf("Корень: x = %.4lf\n", x1); break;
                    case 0:  printf("Нет вещественных корней\n"); break;
                    case -1: printf("Бесконечно много решений\n"); break;
                }
            }
        }
    }
    return STATUS_OK;
}

static void print_error_message(status_code_t code)
{
    switch (code)
    {
    case STATUS_ERROR_INVALID_ARG_COUNT:
        fprintf(stderr, "Ошибка: Неверное количество аргументов командной строки.\n");
        break;
    case STATUS_ERROR_INVALID_OPTION:
        fprintf(stderr, "Ошибка: Неизвестный или некорректный формат флага.\n");
        break;
    case STATUS_ERROR_INVALID_ARG_VALUE:
        fprintf(stderr, "Ошибка: Переданный аргумент имеет неверное значение или формат.\n");
        break;
    case STATUS_ERROR_INVALID_EPSILON:
        fprintf(stderr, "Ошибка: Эпсилон (eps) должен быть строго больше 0.\n");
        break;
    case STATUS_ERROR_DIVISION_BY_ZERO:
        fprintf(stderr, "Ошибка: Попытка деления на ноль.\n");
        break;
    case STATUS_ERROR_NULL_POINTER:
        fprintf(stderr, "Ошибка: В функцию был передан пустой указатель (NULL).\n");
        break;
    default:
        fprintf(stderr, "Произошла неизвестная ошибка (Код: %d).\n", code);
        break;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        print_error_message(STATUS_ERROR_INVALID_ARG_COUNT);
        fprintf(stderr, "Использование:\n");
        fprintf(stderr, "  %s <-m|/m> <a:int> <b:int>\n", argv[0]);
        fprintf(stderr, "  %s <-t|/t> <eps:double> <a:double> <b:double> <c:double>\n", argv[0]);
        fprintf(stderr, "  %s <-q|/q> <eps:double> <a:double> <b:double> <c:double>\n", argv[0]);
        return STATUS_ERROR_INVALID_ARG_COUNT;
    }

    const char *flag_str = argv[1];

    if ((flag_str[0] != '-' && flag_str[0] != '/') || flag_str[1] == '\0' || flag_str[2] != '\0')
    {
        print_error_message(STATUS_ERROR_INVALID_OPTION);
        return STATUS_ERROR_INVALID_OPTION;
    }

    char flag = flag_str[1];
    switch (flag)
    {
        case 'm':
        {
            if (argc != 4)
            {
                print_error_message(STATUS_ERROR_INVALID_ARG_COUNT);
                return STATUS_ERROR_INVALID_ARG_COUNT;
            }

            long long a = 0, b = 0;
            status_code_t status = parser_long_long_data_type(argv[2], &a);
            if (status != STATUS_OK)
            {
                print_error_message(status);
                return status;
            }

            status = parser_long_long_data_type(argv[3], &b);
            if (status != STATUS_OK)
            {
                print_error_message(status);
                return status;
            }

            bool is_mult = false;
            status = flag_m(a, b, &is_mult);
            if (status != STATUS_OK)
            {
                print_error_message(status);
                return status;
            }

            if (is_mult)
            {
                printf("Число %lld кратно %lld.\n", a, b);
            }
            else
            {
                printf("Число %lld НЕ кратно %lld.\n", a, b);
            }
            break;
        }

        case 't':
        {
            if (argc != 6)
            {
                print_error_message(STATUS_ERROR_INVALID_ARG_COUNT);
                return STATUS_ERROR_INVALID_ARG_COUNT;
            }

            double eps = 0.0, a = 0.0, b = 0.0, c = 0.0;
            if (parser_double_data_type(argv[2], &eps) != STATUS_OK ||
                parser_double_data_type(argv[3], &a) != STATUS_OK ||
                parser_double_data_type(argv[4], &b) != STATUS_OK ||
                parser_double_data_type(argv[5], &c) != STATUS_OK)
            {
                print_error_message(STATUS_ERROR_INVALID_ARG_VALUE);
                return STATUS_ERROR_INVALID_ARG_VALUE;
            }

            bool is_right = false;
            status_code_t status = flag_t(eps, a, b, c, &is_right);
            if (status != STATUS_OK)
            {
                print_error_message(status);
                return status;
            }

            if (is_right)
            {
                printf("Стороны %.4lf, %.4lf, %.4lf ОБРАЗУЮТ прямоугольный треугольник.\n", a, b, c);
            }
            else
            {
                printf("Стороны %.4lf, %.4lf, %.4lf НЕ образуют прямоугольный треугольник.\n", a, b, c);
            }
            break;
        }

        case 'q':
        {
            if (argc != 6)
            {
                print_error_message(STATUS_ERROR_INVALID_ARG_COUNT);
                return STATUS_ERROR_INVALID_ARG_COUNT;
            }

            double eps = 0.0, a = 0.0, b = 0.0, c = 0.0;
            if (parser_double_data_type(argv[2], &eps) != STATUS_OK ||
                parser_double_data_type(argv[3], &a) != STATUS_OK ||
                parser_double_data_type(argv[4], &b) != STATUS_OK ||
                parser_double_data_type(argv[5], &c) != STATUS_OK)
            {
                print_error_message(STATUS_ERROR_INVALID_ARG_VALUE);
                return STATUS_ERROR_INVALID_ARG_VALUE;
            }

            status_code_t status = flag_q(eps, a, b, c);
            if (status != STATUS_OK)
            {
                print_error_message(status);
                return status;
            }
            break;
        }

        default:
            print_error_message(STATUS_ERROR_INVALID_OPTION);
            return STATUS_ERROR_INVALID_OPTION;
    }

    return STATUS_OK;
}