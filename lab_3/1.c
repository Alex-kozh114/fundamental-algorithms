#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>

typedef enum
{
    STATUS_OK,
    STATUS_ERROR_NULL_POINTER,
    STATUS_ERROR_OUT_OF_MEMORY,
    STATUS_ERROR_INVALID_ARGUMENT,
    STATUS_ERROR_INVALID_BASE,
    STATUS_ERROR_INVALID_NUMBER_FORMAT,
    STATUS_ERROR_OVERFLOW
} status_code_t;

#define MIN_POWER 1u
#define MAX_POWER 5u

static unsigned long long bitwise_add(const unsigned long long a, const unsigned long long b)
{
    unsigned long long sum = a;
    unsigned long long carry = b;
    while (carry != 0)
    {
        const unsigned long long next_carry = (sum & carry) << 1;
        sum ^= carry;
        carry = next_carry;
    }
    return sum;
}

static unsigned long long bitwise_negate(const unsigned long long value)
{
    return bitwise_add(~value, 1ULL);
}

static size_t bitwise_increment(const size_t value)
{
    return (size_t)bitwise_add((unsigned long long)value, 1ULL);
}

static size_t bitwise_decrement(const size_t value)
{
    return (size_t)bitwise_add((unsigned long long)value, ~0ULL);
}

static status_code_t convert_to_base_pow2(const long long number, const unsigned int r, char **out_str)
{
    static const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";

    if (!out_str)
        return STATUS_ERROR_NULL_POINTER;
    if (r < MIN_POWER || r > MAX_POWER)
        return STATUS_ERROR_INVALID_BASE;

    const bool is_negative = number < 0;
    unsigned long long magnitude = (unsigned long long)number;
    if (is_negative)
    {
        magnitude = bitwise_negate(magnitude);
    }

    const unsigned long long mask = ~(~0ULL << r);

    size_t digit_count = 0;
    unsigned long long tmp = magnitude;
    do
    {
        digit_count = bitwise_increment(digit_count);
        tmp >>= r;
    } while (tmp != 0);

    size_t str_len = digit_count;
    if (is_negative)
        str_len = bitwise_increment(str_len);

    char *buf = (char *)malloc(bitwise_increment(str_len));
    if (!buf)
        return STATUS_ERROR_OUT_OF_MEMORY;

    buf[str_len] = '\0';
    size_t pos = str_len;
    tmp = magnitude;
    do
    {
        pos = bitwise_decrement(pos);
        buf[pos] = digits[tmp & mask];
        tmp >>= r;
    } while (tmp != 0);

    if (is_negative)
        buf[0] = '-';

    *out_str = buf;
    return STATUS_OK;
}

static status_code_t parser_long_long_data_type(const char *str, long long *result)
{
    if (str == NULL || result == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }

    while (isspace((unsigned char)*str))
    {
        str++;
    }

    if (*str == '\0')
    {
        return STATUS_ERROR_INVALID_ARGUMENT;
    }

    const char *p = str;
    bool negative = false;

    if (*p == '-' || *p == '+')
    {
        negative = (*p == '-');
        p++;
    }

    if (!isdigit((unsigned char)*p))
    {
        return STATUS_ERROR_INVALID_ARGUMENT;
    }

    while (*p == '0')
    {
        p++;
    }

    size_t digits_count = 0;
    const char *start_digits = p;
    while (isdigit((unsigned char)*p))
    {
        digits_count++;
        p++;
    }

    if (*p != '\0')
    {
        return STATUS_ERROR_INVALID_NUMBER_FORMAT;
    }

    if (digits_count > 19)
    {
        return STATUS_ERROR_OVERFLOW;
    }
    if (digits_count == 19)
    {
        // LLONG_MAX = 9223372036854775807, LLONG_MIN = 9223372036854775808
        const char *limit = negative ? "9223372036854775808" : "9223372036854775807";
        if (strncmp(start_digits, limit, 19) > 0)
        {
            return STATUS_ERROR_OVERFLOW;
        }
    }

    errno = 0;
    char *endptr = NULL;
    const long long value = strtoll(str, &endptr, 10);

    if (errno == ERANGE)
    {
        return STATUS_ERROR_OVERFLOW;
    }
    if (endptr == str || *endptr != '\0')
    {
        return STATUS_ERROR_INVALID_NUMBER_FORMAT;
    }

    *result = value;
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
    case STATUS_ERROR_INVALID_ARGUMENT:
        return "Неверный аргумент";
    case STATUS_ERROR_INVALID_BASE:
        return "Недопустимое значение r (должно быть от 1 до 5)";
    case STATUS_ERROR_INVALID_NUMBER_FORMAT:
        return "Некорректный формат числа";
    case STATUS_ERROR_OVERFLOW:
        return "Число выходит за пределы диапазона long long";
    default:
        return "Неизвестная ошибка";
    }
}

static void print_error(const status_code_t sc)
{
    printf("Ошибка: %s (код: %d)\n", status_to_string(sc), (int)sc);
}

static status_code_t print_conversion(const long long number, const unsigned int r)
{
    char *result = NULL;
    const status_code_t sc = convert_to_base_pow2(number, r, &result);
    if (sc != STATUS_OK)
        return sc;

    printf("  r = %u (основание %2u): %s\n", r, 1u << r, result);
    free(result);
    return STATUS_OK;
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        const long long test_numbers[] = {
            0, 1, -1, 10, 31, 32, 255, -255,
            1024, 123456789, LLONG_MAX, LLONG_MIN};
        const size_t test_count = sizeof(test_numbers) / sizeof(test_numbers[0]);

        for (size_t i = 0; i < test_count; ++i)
        {
            printf("Число %lld:\n", test_numbers[i]);
            for (unsigned int r = MIN_POWER; r <= MAX_POWER; ++r)
            {
                const status_code_t sc = print_conversion(test_numbers[i], r);
                if (sc != STATUS_OK)
                {
                    print_error(sc);
                    return sc;
                }
            }
            printf("\n");
        }

        printf("Проверка обработки ошибок:\n");
        char *dummy = NULL;
        status_code_t sc = convert_to_base_pow2(10, 0u, &dummy);
        printf("  r = 0: ");
        print_error(sc);
        sc = convert_to_base_pow2(10, 6u, &dummy);
        printf("  r = 6: ");
        print_error(sc);
        sc = convert_to_base_pow2(10, 1u, NULL);
        printf("  out_str = NULL: ");
        print_error(sc);

        return STATUS_OK;
    }

    if (argc != 3)
    {
        printf("Использование:\n");
        printf("  %s                 - демонстрация работы\n", argv[0]);
        printf("  %s <число> <r>     - перевод числа в систему с основанием 2^r, r = 1..5\n", argv[0]);
        return STATUS_ERROR_INVALID_ARGUMENT;
    }

    long long number = 0;
    status_code_t sc = parser_long_long_data_type(argv[1], &number);
    if (sc != STATUS_OK)
    {
        print_error(sc);
        return sc;
    }

    long long r_value = 0;
    sc = parser_long_long_data_type(argv[2], &r_value);
    if (sc != STATUS_OK)
    {
        print_error(sc);
        return sc;
    }

    // проверяем диапазон до сужающего каста, иначе 2^32 + 1 превратится в 1
    if (r_value < (long long)MIN_POWER || r_value > (long long)MAX_POWER)
    {
        print_error(STATUS_ERROR_INVALID_BASE);
        return STATUS_ERROR_INVALID_BASE;
    }
    const unsigned int r = (unsigned int)r_value;

    printf("Число %lld:\n", number);
    sc = print_conversion(number, r);
    if (sc != STATUS_OK)
    {
        print_error(sc);
        return sc;
    }

    return STATUS_OK;
}