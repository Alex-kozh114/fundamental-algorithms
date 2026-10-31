#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define MAX_STR_LEN 256

typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_INVALID_ARGS,
    STATUS_ERR_MEMORY,
    STATUS_ERR_INVALID_FORMAT
} status_code_t;

int char_to_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return -1;
}

char val_to_char(int v) {
    if (v >= 0 && v <= 9) return '0' + v;
    if (v >= 10 && v < 36) return 'a' + (v - 10);
    return '0';
}

status_code_t validate_number_string(const char *str, int base) {
    if (str == NULL || str[0] == '\0') {
        return STATUS_ERR_INVALID_FORMAT;
    }
    for (size_t i = 0; str[i] != '\0'; ++i) {
        int val = char_to_val(str[i]);
        if (val < 0 || val >= base) {
            return STATUS_ERR_INVALID_FORMAT;
        }
    }
    return STATUS_OK;
}

status_code_t add_two_numbers(const char *num1, const char *num2, int base, char **out_result) {
    if (num1 == NULL || num2 == NULL || out_result == NULL || base < 2 || base > 36) {
        return STATUS_ERR_INVALID_ARGS;
    }

    status_code_t status1 = validate_number_string(num1, base);
    status_code_t status2 = validate_number_string(num2, base);
    if (status1 != STATUS_OK || status2 != STATUS_OK) {
        return STATUS_ERR_INVALID_FORMAT;
    }

    size_t len1 = strlen(num1);
    size_t len2 = strlen(num2);
    size_t max_len = (len1 > len2) ? len1 : len2;

    char *res = (char *)malloc(max_len + 2);
    if (res == NULL) {
        return STATUS_ERR_MEMORY;
    }

    int carry = 0;
    size_t res_idx = 0;
    long long i = (long long)len1 - 1;
    long long j = (long long)len2 - 1;

    while (i >= 0 || j >= 0 || carry > 0) {
        int sum = carry;
        if (i >= 0) {
            sum += char_to_val(num1[i]);
            i--;
        }
        if (j >= 0) {
            sum += char_to_val(num2[j]);
            j--;
        }
        carry = sum / base;
        int digit = sum % base;
        res[res_idx++] = val_to_char(digit);
    }
    res[res_idx] = '\0';

    for (size_t k = 0; k < res_idx / 2; ++k) {
        char temp = res[k];
        res[k] = res[res_idx - 1 - k];
        res[res_idx - 1 - k] = temp;
    }

    size_t start = 0;
    while (res[start] == '0' && start + 1 < res_idx) {
        start++;
    }

    if (start > 0) {
        size_t new_len = res_idx - start;
        memmove(res, res + start, new_len + 1);
    }

    *out_result = res;
    return STATUS_OK;
}

status_code_t sum_array_in_base(int base, size_t count, const char **numbers, char **out_result) {
    if (base < 2 || base > 36 || out_result == NULL || (count > 0 && numbers == NULL)) {
        return STATUS_ERR_INVALID_ARGS;
    }

    if (count == 0) {
        char *res = (char *)malloc(2);
        if (res == NULL) return STATUS_ERR_MEMORY;
        res[0] = '0';
        res[1] = '\0';
        *out_result = res;
        return STATUS_OK;
    }

    char *current_sum = (char *)malloc(2);
    if (current_sum == NULL) return STATUS_ERR_MEMORY;
    current_sum[0] = '0';
    current_sum[1] = '\0';

    status_code_t status = STATUS_OK;

    for (size_t k = 0; k < count; ++k) {
        if (numbers[k] == NULL) {
            status = STATUS_ERR_INVALID_ARGS;
            free(current_sum);
            break;
        }

        char *new_sum = NULL;
        status = add_two_numbers(current_sum, numbers[k], base, &new_sum);
        free(current_sum);
        if (status != STATUS_OK) {
            break;
        }
        current_sum = new_sum;
    }

    if (status == STATUS_OK) {
        *out_result = current_sum;
    }
    return status;
}

status_code_t sum_in_base(int base, size_t count, char **out_result, ...) {
    if (base < 2 || base > 36 || out_result == NULL) {
        return STATUS_ERR_INVALID_ARGS;
    }
    if (count == 0) {
        return sum_array_in_base(base, 0, NULL, out_result);
    }

    const char **arr = malloc(count * sizeof(const char *));
    if (arr == NULL) {
        return STATUS_ERR_MEMORY;
    }

    va_list args;
    va_start(args, out_result);
    for (size_t i = 0; i < count; ++i) {
        arr[i] = va_arg(args, const char *);
    }
    va_end(args);

    status_code_t status = sum_array_in_base(base, count, arr, out_result);
    free(arr);
    return status;
}

int main(void) {
    char *result = NULL;
    status_code_t status;

    status = sum_in_base(10, 3, &result, "123", "456", "789");
    if (status == STATUS_OK) {
        printf("Base 10 sum (\"1203\", \"456\", \"789\") = %s\n", result);
        free(result);
    }

    status = sum_in_base(16, 4, &result, "a", "f", "10", "ff");
    if (status == STATUS_OK) {
        printf("Base 16 sum (\"a\", \"f\", \"10\", \"ff\") = %s\n", result);
        free(result);
    }

    status = sum_in_base(2, 3, &result, "1", "1", "0");
    if (status == STATUS_OK) {
        printf("Base 2 sum (\"1\", \"1\", \"0\") = %s\n", result);
        free(result);
    }
    status = sum_in_base(4, 3, &result, "12200000000000000000000000000000000000", "1", "0");
    if (status == STATUS_OK) {
        printf("Base 4 sum (\"1\", \"1\", \"0\") = %s\n", result);
        free(result);
    }

    return 0;
}