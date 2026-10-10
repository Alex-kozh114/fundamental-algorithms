#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

#define MAX_ITERATIONS 10000000

typedef enum
{
    STATUS_OK,
    STATUS_ERROR_NULL_POINTER,
    STATUS_ERROR_INVALID_ARG_COUNT,
    STATUS_ERROR_INVALID_OPTION,
    STATUS_ERROR_INVALID_ARG_VALUE,
    STATUS_ERROR_INVALID_FORMAT,
    STATUS_ERROR_INVALID_PARAMETER,
    STATUS_ERROR_OUT_OF_RANGE,
    STATUS_ERROR_MATH_OVERFLOW,
    STATUS_ERROR_MEMORY_ALLOC,
    STATUS_ERROR_MEMORY_REALLOC
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

static status_code_t flag_h(const long long x, long long **result, size_t *count)
{
    if (result == NULL || count == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    if (x <= 0 || x > 100)
    {
        return STATUS_ERROR_INVALID_ARG_VALUE;
    }

    long long *arr = malloc(100 * sizeof(long long));
    if (arr == NULL)
    {
        return STATUS_ERROR_MEMORY_ALLOC;
    }

    size_t idx = 0;
    for (long long i = x; i <= 100; i += x)
    {
        arr[idx] = i;
        idx++;
    }

    *result = arr;
    *count = idx;
    return STATUS_OK;
}

static status_code_t flag_p(const long long x, bool *is_prime, bool *is_composite)
{
    if (is_prime == NULL || is_composite == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    if (x <= 0)
    {
        return STATUS_ERROR_INVALID_ARG_VALUE;
    }
    if (x == 1)
    {
        *is_prime = false;
        *is_composite = false;
        return STATUS_OK;
    }
    if (x > 1)
    {
        for (long long i = 2; i <= x / 2; ++i)
        {
            if (x % i == 0)
            {
                *is_prime = false;
                *is_composite = true;
                return STATUS_OK;
            }
        }
        *is_prime = true;
        *is_composite = false;
    }
    return STATUS_OK;
}

static status_code_t flag_a(const long long x, long long *result)
{
    if (result == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    if (x <= 0)
    {
        return STATUS_ERROR_INVALID_ARG_VALUE;
    }
    long long sum = 0;
    sum = (x * (x + 1)) / 2;
    *result = sum;
    return STATUS_OK;
}

static status_code_t flag_f(const long long x, long long *result)
{
    if (result == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    if (x < 0)
    {
        return STATUS_ERROR_INVALID_ARG_VALUE;
    }
    if (x > 20)
    {
        return STATUS_ERROR_OUT_OF_RANGE;
    }
    long long fact = 1;
    for (long long i = 1; i <= x; ++i)
    {
        fact *= i;
    }
    *result = fact;
    return STATUS_OK;
}

static status_code_t flag_e(const long long x, long long table[10][10])
{
    if (table == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    if (x < 1 || x > 10)
    {
        return STATUS_ERROR_INVALID_PARAMETER;
    }

    for (int base = 1; base <= 10; ++base)
    {
        long long current = 1;
        for (int pow = 1; pow <= x; ++pow)
        {
            if (current > LLONG_MAX / base)
            {
                return STATUS_ERROR_MATH_OVERFLOW;
            }
            current *= base;
            table[base - 1][pow - 1] = current;
        }
    }

    return STATUS_OK;
}

static status_code_t flag_s(const long long x, char **hex_digits, size_t *count)
{
    if (hex_digits == NULL || count == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    if (x < 0)
    {
        return STATUS_ERROR_INVALID_PARAMETER;
    }
    // llong_max в 16 ричной - FFFFFFFFFFFFFFFF
    char *digits = (char *)malloc(32 * sizeof(char));
    if (digits == NULL)
    {
        return STATUS_ERROR_MEMORY_ALLOC;
    }

    const char *hex_map = "0123456789ABCDEF";

    if (x == 0)
    {
        digits[0] = '0';
        *count = 1;
        *hex_digits = digits;
        return STATUS_OK;
    }

    size_t len = 0;
    long long temp_x = x;

    while (temp_x > 0)
    {
        int y = (int)(temp_x % 16);
        digits[len++] = hex_map[y];
        temp_x /= 16;
    }

    for (size_t i = 0; i < len / 2; ++i)
    {
        char tmp = digits[i];
        digits[i] = digits[len - 1 - i];
        digits[len - 1 - i] = tmp;
    }

    *hex_digits = digits;
    *count = len;
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
    case STATUS_ERROR_INVALID_FORMAT:
        fprintf(stderr, "Ошибка: Переданное значение x некорректно для данной операции.\n");
        break;
    case STATUS_ERROR_INVALID_PARAMETER:
        fprintf(stderr, "Ошибка: Для флага -e/e значение x должно быть в диапазоне 1..10.\n");
        break;
    case STATUS_ERROR_OUT_OF_RANGE:
        fprintf(stderr, "Ошибка: Значение выходит за пределы допустимого диапазона.\n");
        break;
    case STATUS_ERROR_MATH_OVERFLOW:
        fprintf(stderr, "Ошибка: Произошло математическое переполнение при вычислениях.\n");
        break;
    case STATUS_ERROR_MEMORY_ALLOC:
    case STATUS_ERROR_MEMORY_REALLOC:
        fprintf(stderr, "Ошибка: Не удалось выделить необходимый объем динамической памяти.\n");
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
    if (argc != 3)
    {
        print_error_message(STATUS_ERROR_INVALID_ARG_COUNT);
        fprintf(stderr, "Использование: %s <число_x> <-флаг|/флаг>\n", argv[0]);
        return STATUS_ERROR_INVALID_ARG_COUNT;
    }

    const char *x_str = argv[1];
    const char *flag_str = argv[2];

    if ((flag_str[0] != '-' && flag_str[0] != '/') || flag_str[1] == '\0' || flag_str[2] != '\0')
    {
        print_error_message(STATUS_ERROR_INVALID_OPTION);
        return STATUS_ERROR_INVALID_OPTION;
    }

    long long x = 0;
    status_code_t status = parser_long_long_data_type(x_str, &x);
    if (status != STATUS_OK)
    {
        print_error_message(status);
        return status;
    }

    char flag = flag_str[1];
    switch (flag)
    {
    case 'h':
    {
        long long *results = NULL;
        size_t count = 0;

        status = flag_h(x, &results, &count);
        if (status != STATUS_OK)
        {
            print_error_message(status);
            return status;
        }

        if (count == 0)
        {
            printf("В пределах 100 нет натуральных чисел, кратных %lld.\n", x);
        }
        else
        {
            printf("Натуральные числа <= 100, кратные %lld:\n", x);
            for (size_t i = 0; i < count; ++i)
            {
                printf("%lld ", results[i]);
            }
            printf("\n");
        }

        free(results);
        break;
    }

    case 'p':
    {
        bool is_prime = false;
        bool is_composite = false;

        status = flag_p(x, &is_prime, &is_composite);
        if (status != STATUS_OK)
        {
            print_error_message(status);
            return status;
        }

        if (!is_prime && !is_composite)
        {
            printf("Число %lld не является ни простым, ни составным.\n", x);
        }
        else if (is_prime)
        {
            printf("Число %lld является простым.\n", x);
        }
        else
        {
            printf("Число %lld является составным.\n", x);
        }
        break;
    }

    case 's':
    {
        char *digits = NULL;
        size_t count = 0;

        status = flag_s(x, &digits, &count);
        if (status != STATUS_OK)
        {
            print_error_message(status);
            return status;
        }

        printf("16-ричные цифры числа %lld : ", x);
        for (size_t i = 0; i < count; ++i)
        {
            printf("%c", digits[i]);
            if (i + 1 < count)
            {
                printf(" ");
            }
        }
        printf("\n");

        free(digits);
        break;
    }

    case 'e':
    {
        long long table[10][10];

        status = flag_e(x, table);
        if (status != STATUS_OK)
        {
            print_error_message(status);
            return status;
        }

        printf("Таблица степеней (показатели 1..%lld):\n", x);
        for (int base = 1; base <= 10; ++base)
        {
            printf("Основание %2d: ", base);
            for (int pow = 1; pow <= x; ++pow)
            {
                printf("%11lld ", table[base - 1][pow - 1]);
            }
            printf("\n");
        }
        break;
    }

    case 'a':
    {
        long long sum = 0;

        status = flag_a(x, &sum);
        if (status != STATUS_OK)
        {
            print_error_message(status);
            return status;
        }

        printf("Сумма натуральных чисел от 1 до %lld равна: %lld\n", x, sum);
        break;
    }

    case 'f':
    {
        long long fact = 0;

        status = flag_f(x, &fact);
        if (status != STATUS_OK)
        {
            print_error_message(status);
            return status;
        }

        printf("Факториал числа %lld равен: %lld\n", x, fact);
        break;
    }

    default:
        print_error_message(STATUS_ERROR_INVALID_OPTION);
        return STATUS_ERROR_INVALID_OPTION;
    }

    return STATUS_OK;
}