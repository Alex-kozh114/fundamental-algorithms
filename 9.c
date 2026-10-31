#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <stdarg.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_INVALID_ARGS,
    STATUS_ERR_MEMORY,
    STATUS_ERR_INVALID_FORMAT
} status_code_t;

bool is_finite_in_base(double x, int base) {
    if (x <= 0.0 || x >= 1.0 || base < 2 || base > 36) {
        return false;
    }

    double current = x;
    const int max_iter = 53;
    const double eps = 1e-11;

    for (int i = 0; i < max_iter; ++i) {
        double int_part;
        current = current * base;
        double frac_part = modf(current, &int_part);

        if (frac_part < eps || frac_part > 1.0 - eps) {
            return true;
        }

        current = frac_part;
    }
    return false;
}

status_code_t check_fractions_array_finite(int base, size_t count, const double *values) {
    if (base < 2 || base > 36 || (count > 0 && values == NULL)) {
        return STATUS_ERR_INVALID_ARGS;
    }

    if (count == 0) {
        printf("Дроби не переданы.\n");
        return STATUS_OK;
    }

    for (size_t i = 0; i < count; ++i) {
        double val = values[i];

        if (val <= 0.0 || val >= 1.0) {
            printf("Ошибка: Значение %.6f под номером %zu вне диапазона (0; 1).\n", val, i + 1);
            continue;
        }

        bool finite = is_finite_in_base(val, base);
        printf("Дробь: %.10f | Основание: %d -> %s\n", 
               val, base, finite ? "Конечное" : "Бесконечное");
    }

    return STATUS_OK;
}

status_code_t check_fractions_finite(int base, size_t count, ...) {
    if (base < 2 || base > 36) {
        return STATUS_ERR_INVALID_ARGS;
    }
    if (count == 0) {
        return check_fractions_array_finite(base, 0, NULL);
    }

    double *arr = (double *)malloc(count * sizeof(double));
    if (arr == NULL) {
        return STATUS_ERR_MEMORY;
    }

    va_list args;
    va_start(args, count);
    for (size_t i = 0; i < count; ++i) {
        arr[i] = va_arg(args, double);
    }
    va_end(args);

    status_code_t status = check_fractions_array_finite(base, count, arr);
    free(arr);
    return status;
}

int main(void) {
    int base = 0;
    printf("Введите основание системы счисления [2..36]: ");
    if (scanf("%d", &base) != 1 || base < 2 || base > 36) {
        printf("Ошибка: Введено неверное основание. Должно быть от 2 до 36.\n");
        return 1;
    }

    size_t count = 0;
    printf("Введите количество дробей для проверки: ");
    if (scanf("%zu", &count) != 1) {
        printf("Ошибка: Введено неверное количество.\n");
        return 1;
    }

    if (count == 0) {
        printf("Нечего проверять (количество равно 0).\n");
        return 0;
    }

    double *fractions = (double *)malloc(count * sizeof(double));
    if (fractions == NULL) {
        printf("Ошибка: Не удалось выделить память.\n");
        return 1;
    }

    for (size_t i = 0; i < count; ++i) {
        printf("Введите дробь %zu (строго между 0 и 1): ", i + 1);
        if (scanf("%lf", &fractions[i]) != 1) {
            printf("Ошибка: Не удалось прочитать значение дроби.\n");
            free(fractions);
            return 1;
        }

        if (fractions[i] <= 0.0 || fractions[i] >= 1.0) {
            printf("Предупреждение: Дробь %.4f вне диапазона (0; 1). Она будет пропущена при обработке.\n", fractions[i]);
        }
    }

    printf("\n Обработка введенных данных\n");
    status_code_t status = check_fractions_array_finite(base, count, fractions);

    free(fractions);

    if (status != STATUS_OK) {
        printf("Ошибка: Вычисление завершилось с кодом %d.\n", (int)status);
        return 1;
    }

    return 0;
}