#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>
#include <string.h>
#define ARRAY_SIZE 10

typedef enum {
    STATUS_OK,
    STATUS_ERROR_NULL_POINTER,
    STATUS_ERROR_INVALID_ARG_VALUE,
    STATUS_ERROR_INVALID_ARG_COUNT,
    STATUS_ERROR_INVALID_FORMAT,
    STATUS_ERROR_INVALID_PARAMETER,
    STATUS_ERROR_OUT_OF_RANGE
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

static status_code_t swap_min_max(int *arr, size_t size)
{
    if (arr == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    if (size == 0)
    {
        return STATUS_ERROR_INVALID_ARG_VALUE;
    }

    size_t min_index = 0;
    size_t max_index = 0;
    for (size_t i = 1; i < size; ++i)
    {
        if (arr[i] < arr[min_index])
        {
            min_index = i;
        }
        if (arr[i] > arr[max_index])
        {
            max_index = i;
        }
    }
    int temp = arr[min_index];
    arr[min_index] = arr[max_index];
    arr[max_index] = temp;
    return STATUS_OK;
}

static void print_array(const int *arr, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

static void print_error(status_code_t code)
{
    switch (code)
    {
    case STATUS_ERROR_INVALID_ARG_COUNT:
        fprintf(stderr, "Ошибка: Неверное количество аргументов.\n");
        break;
    case STATUS_ERROR_INVALID_FORMAT:
    case STATUS_ERROR_INVALID_ARG_VALUE:
        fprintf(stderr, "Ошибка: Переданные границы [a, b] должны быть целыми числами.\n");
        break;
    case STATUS_ERROR_INVALID_PARAMETER:
        fprintf(stderr, "Ошибка: Граница 'a' не может быть больше 'b'.\n");
        break;
    case STATUS_ERROR_NULL_POINTER:
        fprintf(stderr, "Ошибка: Передан пустой указатель (NULL).\n");
        break;
    default:
        fprintf(stderr, "Произошла ошибка с кодом %d.\n", code);
        break;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        print_error(STATUS_ERROR_INVALID_ARG_COUNT);
        fprintf(stderr, "Использование: %s <a> <b>\n", argv[0]);
        return STATUS_ERROR_INVALID_ARG_COUNT;
    }

    long long a_val = 0, b_val = 0;
    status_code_t st_a = parser_long_long_data_type(argv[1], &a_val);
    status_code_t st_b = parser_long_long_data_type(argv[2], &b_val);

    if (st_a != STATUS_OK || st_b != STATUS_OK)
    {
        print_error(STATUS_ERROR_INVALID_FORMAT);
        return STATUS_ERROR_INVALID_FORMAT;
    }

    if (a_val > b_val)
    {
        print_error(STATUS_ERROR_INVALID_PARAMETER);
        return STATUS_ERROR_INVALID_PARAMETER;
    }

    int a = (int)a_val;
    int b = (int)b_val;
    // стартовое значение генератора случайных чисел
    srand((unsigned int)time(NULL));

    int arr[ARRAY_SIZE];
    for (size_t i = 0; i < ARRAY_SIZE; ++i)
    {
        arr[i] = a + rand() % (b - a + 1);
    }

    printf("Исходный массив:\n");
    print_array(arr, ARRAY_SIZE);

    status_code_t status = swap_min_max(arr, ARRAY_SIZE);
    if (status != STATUS_OK)
    {
        print_error(status);
        return status;
    }

    printf("Массив после замены min и max:\n");
    print_array(arr, ARRAY_SIZE);

    return STATUS_OK;
}