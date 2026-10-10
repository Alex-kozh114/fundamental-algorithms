#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
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

#define _USE_MATH_DEFINES

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

static double ln_factorial(int n) {
    double s = 0.0;
    for (int i = 2; i <= n; ++i) {
        s += log((double)i);
    }
    return s;
}

static int is_prime(int n) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    for (int d = 3; d * d <= n; d += 2) {
        if (n % d == 0) return 0;
    }
    return 1;
}

static status_code_t limit_e(double eps, double *out_res) {
    if (out_res == NULL) return STATUS_ERROR_NULL_POINTER;
    if (eps <= 0) return STATUS_ERROR_INVALID_EPSILON;

    double n = 1.0;
    double current = 0.0, previous = 0.0;
    do {
        previous = current;
        n *= 2.0;
        current = pow(1.0 + 1.0 / n, n);
    } while (fabs(previous - current) >= eps);
    *out_res = current;
    return STATUS_OK;
}

static status_code_t row_e(double eps, double *out_res) {
    if (out_res == NULL) return STATUS_ERROR_NULL_POINTER;
    if (eps <= 0) return STATUS_ERROR_INVALID_EPSILON;

    double sum = 1.0;
    double term = 1.0;
    int n = 1;
    while (fabs(term) > eps){
        term *= (1.0 /n);
        sum += term;
        n++;
    }

    *out_res = sum;
    return STATUS_OK;
}
// метод ньютона
static status_code_t equation_e(double eps, double *out_res) {
    if (out_res == NULL) return STATUS_ERROR_NULL_POINTER;
    if (eps <= 0) return STATUS_ERROR_INVALID_EPSILON;

    double x = 2.0;
    double f = 1.0;
    while (fabs(f) > eps) {
        f = log(x) - 1.0;
        x = x - f * x;
    }

    *out_res = x;
    return STATUS_OK;
}

static status_code_t limit_pi(double eps, double *out_res) {
    if (out_res == NULL) return STATUS_ERROR_NULL_POINTER;
    if (eps <= 0) return STATUS_ERROR_INVALID_EPSILON;

    double product = 1.0;
    double prev = 0.0;
    int k = 1;
    double eff_eps = eps * eps;
    do {
        prev = product;
        product *= (2.0 * k * 2.0 * k) / ((2.0 * k - 1.0) * (2.0 * k + 1.0));
        k++;
    } while (fabs(product - prev) >= eff_eps);

    *out_res = 2.0 * product;
    return STATUS_OK;
}

static status_code_t row_pi(double eps, double *out_res) {
    if (out_res == NULL) return STATUS_ERROR_NULL_POINTER;
    if (eps <= 0) return STATUS_ERROR_INVALID_EPSILON;
    double sum = 1.0;  
    double term = 1.0; 
    double negative = -1.0;
    int n = 2;
    while (fabs(term) >= eps) {
        term *= (double)(2 * n - 3) / (double)(2 * n - 1);
        sum += negative * term;
        negative = -negative;
        n++;
    }

    *out_res = 4.0 * sum;
    return STATUS_OK;
}
// метод ньютона падает из-за того, что производная функции cos(x) + 1 равна -sin(x), которая может быть равна нулю, что приводит к делению на ноль.
// возьмем эквивалентное уравнение cos(x/2) = 0;
static status_code_t equation_pi(double eps, double *out_res) {
    if (out_res == NULL) return STATUS_ERROR_NULL_POINTER;
    if (eps <= 0) return STATUS_ERROR_INVALID_EPSILON;

    double x = 3.0; 
    double f = 1.0;

    while (fabs(f) > eps) {
        f = cos(x / 2.0);      
        x = x + 2.0 * f / sin(x / 2.0);
    }

    *out_res = x;
    return STATUS_OK;
}

static status_code_t limit_ln(double eps, double *out_res) {
    if (out_res == NULL) return STATUS_ERROR_NULL_POINTER;
    if (eps <= 0) return STATUS_ERROR_INVALID_EPSILON;

    double n = 1.0;
    double current = 0.0, previous = 0.0;
    do {
        previous = current;
        n *= 2.0;
        current = n * (pow(2.0, 1.0 / n) - 1.0);
    } while (fabs(previous - current) >= eps);

    *out_res = current;
    return STATUS_OK;
}

static status_code_t row_ln(double eps, double *out_res) {
    if (out_res == NULL) return STATUS_ERROR_NULL_POINTER;
    if (eps <= 0) return STATUS_ERROR_INVALID_EPSILON;

    double sum = 0.0;
    double term = 1.0;  
    double negative = 1.0;
    int n = 1;

    while (fabs(term) >= eps) {
        sum += negative * term;
        negative = -negative;
        n++;
        term *= (double)(n - 1) / (double)n;
    }

    *out_res = sum;
    return STATUS_OK;
}

static status_code_t equation_ln(double eps, double *out_res) {
    if (out_res == NULL) return STATUS_ERROR_NULL_POINTER;
    if (eps <= 0) return STATUS_ERROR_INVALID_EPSILON;

    double x = 1.0;
    double f = 1.0;
    while (fabs(f) > eps) {
        f = exp(x) - 2.0;
        x = x - f / exp(x);
    }

    *out_res = x;
    return STATUS_OK;
}

static status_code_t limit_sqrt(double eps, double *out_res) {
    if (out_res == NULL) return STATUS_ERROR_NULL_POINTER;
    if (eps <= 0) return STATUS_ERROR_INVALID_EPSILON;

    double current = -0.5;
    double previous = 0.0;
    do {
        previous = current;
        current = previous - (previous * previous) / 2.0 + 1.0;
    } while (fabs(current - previous) >= eps);

    *out_res = current;
    return STATUS_OK;
}

static status_code_t row_sqrt(double eps, double *out_res) {
    if (out_res == NULL) return STATUS_ERROR_NULL_POINTER;
    if (eps <= 0) return STATUS_ERROR_INVALID_EPSILON;

    double sum = 0.0;
    double term = 0.25; // при k = 2 

    while (fabs(term) >= eps) {
        sum += term;
        term *= 0.5;
    }
    *out_res = pow(2.0, sum);
    return STATUS_OK;
}

static status_code_t equation_sqrt(double eps, double *out_res) {
    if (out_res == NULL) return STATUS_ERROR_NULL_POINTER;
    if (eps <= 0) return STATUS_ERROR_INVALID_EPSILON;

    double x = 1.0;
    double f = 1.0;
    while (fabs(f) > eps) {
        f = x * x - 2.0;
        x = x - f / (2.0 * x);
    }

    *out_res = x;
    return STATUS_OK;
}
// биноминальный коэффицент вычисляется через рекурентные соотношения
static double limit_gamma_sum(int m) {
    double sum = 0.0;
    double c = 1.0;
    for (int k = 1; k <= m; ++k) {
        c = c * (m - k + 1) / k;
        double negative = (k % 2 == 0) ? 1.0 : -1.0;
        sum += negative * c * ln_factorial(k) / (double)k;
    }
    return sum;
}

static status_code_t limit_gamma(double eps, double *out_res) {
    if (out_res == NULL) return STATUS_ERROR_NULL_POINTER;
    if (eps <= 0) return STATUS_ERROR_INVALID_EPSILON;

    int m = 1;
    double current = 0.0, previous = 0.0;
    do {
        previous = current;
        m++;
        if (m > 20){
            return STATUS_ERROR_OUT_OF_RANGE;
        }
        current = limit_gamma_sum(m);

    } while (fabs(previous - current) >= eps);

    *out_res = current;
    return STATUS_OK;
}

static status_code_t row_gamma(double eps, double *out_res) {
    if (out_res == NULL) return STATUS_ERROR_NULL_POINTER;
    if (eps <= 0) return STATUS_ERROR_INVALID_EPSILON;

    double pi_sq_over_6 = M_PI * M_PI / 6.0;
    double sum = 0.0;
    int k_max = (int)(1.0 / eps) + 10;
    if (k_max > 10000000) k_max = 10000000;

    for (int k = 2; k <= k_max; ++k) {
        double l = floor(sqrt((double)k));
        sum += 1.0 / (l * l) - 1.0 / (double)k;
    }

    *out_res = sum - pi_sq_over_6;
    return STATUS_OK;
}

static status_code_t equation_gamma(double eps, double *out_res) {
    if (out_res == NULL) return STATUS_ERROR_NULL_POINTER;
    if (eps <= 0) return STATUS_ERROR_INVALID_EPSILON;

    long long t = 10;
    double current = 0.0, previous = 0.0;
    do {
        previous = current;
        t *= 2;
        double product = 1.0;
        for (int p = 2; p <= t; ++p) {
            if (is_prime(p)) {
                product *= (p - 1.0) / (double)p;
            }
        }
        current = log((double)t) * product;
    } while (fabs(previous - current) >= eps);

    *out_res = -log(current); // e^-x
    return STATUS_OK;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Используется: %s <eps>\n", argv[0]);
        return STATUS_ERROR_INVALID_ARG_COUNT;
    }

    double eps = 0.0;
    status_code_t err = parser_double_data_type(argv[1], &eps);
    switch (err) {
        case STATUS_OK:
            break;
        case STATUS_ERROR_INVALID_FORMAT:
            fprintf(stderr, "Ошибка: некорректный формат числа '%s'\n", argv[1]);
            return err;
        case STATUS_ERROR_INVALID_EPSILON:
            fprintf(stderr, "Ошибка: значение эпсилон должно быть положительным числом\n");
            return err;
        default:
            fprintf(stderr, "Непредвиденная ошибка при парсинге\n");
            return err;
    }

    double res_lim, res_row, res_eq;

    printf("\t\tПредел\t\tРяд\t\tУравнение\n");

    limit_e(eps, &res_lim);        row_e(eps, &res_row);        equation_e(eps, &res_eq);
    printf("e:       %lf \t%lf \t%lf\n", res_lim, res_row, res_eq);

    limit_pi(eps, &res_lim);       row_pi(eps, &res_row);       equation_pi(eps, &res_eq);
    printf("Pi:      %lf \t%lf \t%lf\n", res_lim, res_row, res_eq);

    limit_ln(eps, &res_lim);       row_ln(eps, &res_row);       equation_ln(eps, &res_eq);
    printf("ln2:     %lf \t%lf \t%lf\n", res_lim, res_row, res_eq);

    limit_sqrt(eps, &res_lim);     row_sqrt(eps, &res_row);     equation_sqrt(eps, &res_eq);
    printf("sqrt(2): %lf \t%lf \t%lf\n", res_lim, res_row, res_eq);

    limit_gamma(eps, &res_lim);    row_gamma(eps, &res_row);    equation_gamma(eps, &res_eq);
    printf("Gamma:   %lf \t%lf \t%lf\n", res_lim, res_row, res_eq);

    return STATUS_OK;
}