#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>

typedef enum
{
    STATUS_OK,
    PARSE_ERROR,
    RANGE_ERROR,
    OVERFLOW_ERROR,
    STATUS_ERROR_INVALID_FORMAT,
    STATUS_ERROR_NULL_POINTER,
    STATUS_ERROR_INVALID_ARG_VALUE,
    STATUS_ERROR_OUT_OF_RANGE,
} status_code_t;

static void print_error(const status_code_t status)
{
    switch (status)
    {
    case STATUS_OK:
        break;
    case STATUS_ERROR_INVALID_ARG_VALUE:
    case STATUS_ERROR_INVALID_FORMAT:
    case PARSE_ERROR:
        printf("Ошибка: Введенное число имеет неверный формат или содержит недопустимые символы.\n");
        break;
    case STATUS_ERROR_OUT_OF_RANGE:
    case RANGE_ERROR:
        printf("Ошибка: Значение выходит за пределы допустимого диапазона .\n");
        break;
    case OVERFLOW_ERROR:
        printf("Ошибка: Произошло математическое переполнение при вычислениях.\n");
        break;
    case STATUS_ERROR_NULL_POINTER:
        printf("Ошибка: Попытка обращения к NULL-указателю.\n");
        break;
    default:
        printf("Произошла неизвестная ошибка (Код: %d).\n", status);
        break;
    }
}

static int char_to_digit(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if (c >= 'A' && c <= 'Z')
    {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'z')
    {
        return c - 'a' + 10;
    }
    return -1;
}

static char digit_to_char(int d)
{
    if (d >= 0 && d <= 9)
    {
        return (char)('0' + d);
    }
    return (char)('A' + (d - 10));
}

static status_code_t parser_long_long_data_type(const char *str, int base, long long *result)
{
    if (str == NULL || result == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }

    while (*str == ' ' || *str == '\n' || *str == '\t' || *str == '\r')
    {
        str++;
    }

    if (*str == '\0')
    {
        return STATUS_ERROR_INVALID_ARG_VALUE;
    }

    if (base < 2 || base > 36)
    {
        return STATUS_ERROR_OUT_OF_RANGE;
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

    if (*p == '\0' || *p == ' ' || *p == '\n' || *p == '\t' || *p == '\r')
    {
        *result = 0;
        return STATUS_OK;
    }

    unsigned long long res = 0;
    size_t digits_count = 0;

    while (*p != '\0' && *p != ' ' && *p != '\n' && *p != '\t' && *p != '\r')
    {
        int d = char_to_digit(*p);
        if (d < 0 || d >= base)
        {
            return STATUS_ERROR_INVALID_FORMAT;
        }

        if (res > (ULLONG_MAX - (unsigned long long)d) / (unsigned long long)base)
        {
            return STATUS_ERROR_OUT_OF_RANGE;
        }

        res = res * (unsigned long long)base + (unsigned long long)d;
        digits_count++;
        p++;
    }

    while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r')
    {
        p++;
    }
    if (*p != '\0')
    {
        return STATUS_ERROR_INVALID_FORMAT;
    }

    if (digits_count == 0)
    {
        return STATUS_ERROR_INVALID_ARG_VALUE;
    }

    if (!negative)
    {
        if (res > (unsigned long long)LLONG_MAX)
        {
            return STATUS_ERROR_OUT_OF_RANGE;
        }
        *result = (long long)res;
    }
    else
    {
        if (res > (unsigned long long)LLONG_MAX + 1ULL)
        {
            return STATUS_ERROR_OUT_OF_RANGE;
        }
        if (res == (unsigned long long)LLONG_MAX + 1ULL)
        {
            *result = LLONG_MIN;
        }
        else
        {
            *result = -(long long)res;
        }
    }

    return STATUS_OK;
}

static status_code_t to_base(long long x, int base, char *buf, int buf_size)
{
    if (buf == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    if (base < 2 || base > 36)
    {
        return STATUS_ERROR_OUT_OF_RANGE;
    }
    if (buf_size < 2)
    {
        return STATUS_ERROR_OUT_OF_RANGE;
    }
    int i = 0;
    unsigned long long n;
    if (x < 0)
    {
        buf[i++] = '-';
        if (x == LLONG_MIN)
        {
            n = (unsigned long long)LLONG_MAX + 1ULL;
        }
        else
        {
            n = (unsigned long long)(-x);
        }
    }
    else
    {
        n = (unsigned long long)x;
    }
    
    if (n == 0)
    {
        if (i >= buf_size - 1)
            return STATUS_ERROR_OUT_OF_RANGE;
        buf[i++] = '0';
    }
    else
    {
        while (n > 0)
        {
            if (i >= buf_size - 1)
            {
                return STATUS_ERROR_OUT_OF_RANGE;
            }
            int digit = (int)(n % (unsigned long long)base);
            buf[i++] = digit_to_char(digit);
            n /= (unsigned long long)base;
        }
    }
    
    int start = (x < 0) ? 1 : 0;
    int end = i - 1;
    while (start < end)
    {
        char tmp = buf[start];
        buf[start] = buf[end];
        buf[end] = tmp;
        start++;
        end--;
    }
    buf[i] = '\0';
    return STATUS_OK;
}

int main(void)
{
    char base_buf[8];
    if (scanf("%7s", base_buf) != 1)
    {
        printf("Ошибка ввода основания\n");
        return STATUS_ERROR_INVALID_FORMAT;
    }
    long long base_ll = 0;
    status_code_t err = parser_long_long_data_type(base_buf, 10, &base_ll);
    if (err != STATUS_OK)
    {
        print_error(err);
        return err;
    }
    if (base_ll < 2 || base_ll > 36)
    {
        printf("Основание должно быть от 2 до 36\n");
        return STATUS_ERROR_OUT_OF_RANGE;
    }
    int base = (int)base_ll;

    long long sum = 0;
    long long max_abs = 0;
    int has_numbers = 0;

    while (1)
    {
        char buf[128];
        if (scanf("%127s", buf) != 1)
        {
            break;
        }
        if (strcasecmp(buf, "stop") == 0)
        {
            break;
        }
        long long num = 0;
        err = parser_long_long_data_type(buf, base, &num);
        if (err != STATUS_OK)
        {
            printf("Число '%s': ", buf);
            print_error(err);
            return err;
        }
        
        if (num > 0 && sum > LLONG_MAX - num)
        {
            printf("Переполнение суммы\n");
            return STATUS_ERROR_OUT_OF_RANGE;
        }
        if (num < 0 && sum < LLONG_MIN - num)
        {
            printf("Переполнение суммы\n");
            return STATUS_ERROR_OUT_OF_RANGE;
        }
        sum += num;

        unsigned long long abs_num;
        if (num == LLONG_MIN)
        {
            abs_num = (unsigned long long)LLONG_MAX + 1ULL;
        }
        else if (num < 0)
        {
            abs_num = (unsigned long long)(-num);
        }
        else
        {
            abs_num = (unsigned long long)num;
        }

        unsigned long long abs_max;
        if (max_abs == LLONG_MIN)
        {
            abs_max = (unsigned long long)LLONG_MAX + 1ULL;
        }
        else if (max_abs < 0)
        {
            abs_max = (unsigned long long)(-max_abs);
        }
        else
        {
            abs_max = (unsigned long long)max_abs;
        }

        if (!has_numbers || abs_num > abs_max)
        {
            max_abs = num;
        }
        has_numbers = 1;
    }

    if (!has_numbers)
    {
        printf("Нет чисел\n");
        return STATUS_ERROR_INVALID_FORMAT;
    }

    int bases[5] = {base, 9, 18, 27, 36};

    printf("Максимум по модулю: %lld\n", max_abs);
    for (int i = 0; i < 5; ++i)
    {
        char buf[128];
        err = to_base(max_abs, bases[i], buf, sizeof(buf));
        if (err == STATUS_OK)
        {
            printf("base %d: %s\n", bases[i], buf);
        }
        else
        {
            printf("base %d: ошибка\n", bases[i]);
        }
    }

    printf("\nСумма: %lld\n", sum);
    for (int i = 0; i < 5; ++i)
    {
        char buf[128];
        err = to_base(sum, bases[i], buf, sizeof(buf));
        if (err == STATUS_OK)
        {
            printf("base %d: %s\n", bases[i], buf);
        }
        else
        {
            printf("base %d: ошибка\n", bases[i]);
        }
    }

    return STATUS_OK;
}
