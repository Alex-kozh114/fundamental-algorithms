#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef enum
{
    STATUS_OK,
    INVALID_PARAMETER,
    MEMORY_ALLOCATION_ERROR,
    RANGE_ERROR
} status_code_t;

static status_code_t is_convex(int n, int *out, ...)
{
    if (out == NULL)
    {
        return INVALID_PARAMETER;
    }
    if (n < 3)
    {
        return RANGE_ERROR;
    }

    va_list ap;
    va_start(ap, out);

    double *xs = (double *)malloc(n * sizeof(double));
    double *ys = (double *)malloc(n * sizeof(double));
    if (xs == NULL || ys == NULL)
    {
        free(xs);
        free(ys);
        va_end(ap);
        return MEMORY_ALLOCATION_ERROR;
    }

    for (int i = 0; i < n; ++i)
    {
        xs[i] = va_arg(ap, double);
        ys[i] = va_arg(ap, double);
    }
    va_end(ap);

    int sign = 0;
    status_code_t status = STATUS_OK;

    for (int i = 0; i < n; ++i)
    {
        int i1 = (i + 1) % n;
        int i2 = (i + 2) % n;

        double abx = xs[i1] - xs[i];
        double aby = ys[i1] - ys[i];
        double bcx = xs[i2] - xs[i1];
        double bcy = ys[i2] - ys[i1];

        double cross = abx * bcy - aby * bcx;

        if (fabs(cross) > 1e-9)
        {
            if (cross > 0)
            {
                if (sign == 0)
                    sign = 1;
                else if (sign == -1)
                {
                    *out = 0;
                    goto cleanup;
                }
            }
            else
            {
                if (sign == 0)
                    sign = -1;
                else if (sign == 1)
                {
                    *out = 0;
                    goto cleanup;
                }
            }
        }
    }

    *out = (sign != 0) ? 1 : 0;

cleanup:
    free(xs);
    free(ys);
    return status;
}

static status_code_t polynomial(double x, int n, double *out, ...)
{
    if (out == NULL)
    {
        return INVALID_PARAMETER;
    }
    if (n < 0)
    {
        return RANGE_ERROR;
    }

    va_list ap;
    va_start(ap, out);

    double result = va_arg(ap, double); // Старший коэффициент a_n
    for (int i = 0; i < n; ++i)
    {
        double coef = va_arg(ap, double);
        result = result * x + coef;
    }
    va_end(ap);

    *out = result;
    return STATUS_OK;
}

static int char_to_digit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 10;
    return -1;
}

static bool is_kaprekar_base(unsigned long long n, int base)
{
    if (n == 1)
        return true;
    if (n == 0)
        return false;

    unsigned long long sq = n * n;
    unsigned long long beta = 1;

    while (beta <= sq / base)
    {
        beta *= (unsigned long long)base;
        unsigned long long L = sq / beta;
        unsigned long long R = sq % beta;

        if (R > 0 && (L + R == n))
        {
            return true;
        }
    }
    return false;
}

status_code_t count_kaprekar(int base, int count, int *out_count, ...)
{
    if (out_count == NULL)
    {
        return INVALID_PARAMETER;
    }
    if (base < 2 || base > 36 || count < 0)
    {
        return RANGE_ERROR;
    }

    va_list ap;
    va_start(ap, out_count);

    int kaprekar_cnt = 0;

    for (int i = 0; i < count; ++i)
    {
        const char *s = va_arg(ap, const char *);
        if (s == NULL)
        {
            va_end(ap);
            return INVALID_PARAMETER;
        }

        unsigned long long n = 0;
        int len = 0;

        for (int j = 0; s[j] != '\0'; ++j)
        {
            int d = char_to_digit(s[j]);
            if (d < 0 || d >= base)
            {
                va_end(ap);
                return INVALID_PARAMETER;
            }
            n = n * (unsigned long long)base + (unsigned long long)d;
            len++;
        }

        if (len == 0)
        {
            va_end(ap);
            return INVALID_PARAMETER;
        }

        if (is_kaprekar_base(n, base))
        {
            kaprekar_cnt++;
        }
    }

    va_end(ap);
    *out_count = kaprekar_cnt;
    return STATUS_OK;
}

static status_code_t geo_mean(double *out, int count, ...)
{
    if (out == NULL)
    {
        return INVALID_PARAMETER;
    }
    if (count <= 0)
    {
        return RANGE_ERROR;
    }

    va_list ap;
    va_start(ap, count);

    double sum_ln = 0.0;
    for (int i = 0; i < count; ++i)
    {
        double val = va_arg(ap, double);
        if (val <= 0.0)
        {
            va_end(ap);
            return RANGE_ERROR;
        }
        sum_ln += log(val);
    }
    va_end(ap);

    *out = exp(sum_ln / count);
    return STATUS_OK;
}

static status_code_t power(double x, int n, double *out)
{
    if (out == NULL)
    {
        return INVALID_PARAMETER;
    }
    if (fabs(x) < 1e-12 && n <= 0)
    {
        return RANGE_ERROR;
    }

    if (n == 0)
    {
        *out = 1.0;
        return STATUS_OK;
    }

    if (n < 0)
    {
        double temp = 0.0;
        status_code_t st = power(x, -n, &temp);
        if (st != STATUS_OK)
            return st;
        *out = 1.0 / temp;
        return STATUS_OK;
    }

    if (n % 2 == 0)
    {
        double half = 0.0;
        status_code_t st = power(x, n / 2, &half);
        if (st != STATUS_OK)
            return st;
        *out = half * half;
        return STATUS_OK;
    }
    else
    {
        double prev = 0.0;
        status_code_t st = power(x, n - 1, &prev);
        if (st != STATUS_OK)
            return st;
        *out = x * prev;
        return STATUS_OK;
    }
}

static status_code_t dichotomy(double a, double b, double eps, double (*f)(double), double *out)
{
    if (out == NULL || f == NULL)
    {
        return INVALID_PARAMETER;
    }
    if (eps <= 0.0 || a >= b)
    {
        return RANGE_ERROR;
    }

    double fa = f(a);
    double fb = f(b);

    if (fa * fb > 0.0)
    {
        return RANGE_ERROR;
    }

    while ((b - a) / 2.0 > eps)
    {
        double mid = a + (b - a) / 2.0;
        double fm = f(mid);

        if (fabs(fm) < 1e-12)
        {
            *out = mid;
            return STATUS_OK;
        }

        if (fa * fm < 0.0)
        {
            b = mid;
            fb = fm;
        }
        else
        {
            a = mid;
            fa = fm;
        }
    }

    *out = a + (b - a) / 2.0;
    return STATUS_OK;
}

static double f_sqrt2(double x) { return x * x - 2.0; }
static double f_trig(double x) { return cos(x) - x; }

int main(void)
{
    status_code_t status;

    printf("1. Проверка выпуклости\n");
    int is_conv = 0;
    status = is_convex(4, &is_conv, 0.0, 0.0, 0.0, 2.0, 2.0, 2.0, 2.0, 0.0);
    if (status == STATUS_OK)
    {
        printf("Квадрат: %s\n\n", is_conv ? "Выпуклый" : "Невыпуклый");
    }

    printf("2. Схема Горнера\n");
    double poly_val = 0.0;
    // P(2.0) = 2x^3 - 3x^2 + 4x - 5
    status = polynomial(2.0, 3, &poly_val, 2.0, -3.0, 4.0, -5.0);
    if (status == STATUS_OK)
    {
        printf("P(2.0) = %lf\n\n", poly_val);
    }

    printf("3. Числа Капрекара\n");
    int kap_cnt = 0;
    status = count_kaprekar(10, 3, &kap_cnt, "45", "10", "297");
    if (status == STATUS_OK)
    {
        printf("Найдено чисел Капрекара (base 10): %d\n\n", kap_cnt);
    }

    printf("4. Среднее геометрическое\n");
    double g_mean = 0.0;
    status = geo_mean(&g_mean, 4, 2.0, 4.0, 8.0, 16.0);
    if (status == STATUS_OK)
    {
        printf("Среднее геометрическое: %lf\n\n", g_mean);
    }

    printf("5. Быстрое возведение в степень \n");
    double pow_val = 0.0;
    if (power(2.0, 10, &pow_val) == STATUS_OK)
    {
        printf("2.0 ^ 10 = %lf\n", pow_val);
    }
    if (power(2.0, -3, &pow_val) == STATUS_OK)
    {
        printf("2.0 ^ -3 = %lf\n\n", pow_val);
    }

    printf("6. Метод дихотомии\n");
    double root = 0.0;
    if (dichotomy(0.0, 2.0, 1e-6, f_sqrt2, &root) == STATUS_OK)
    {
        printf("Корень x^2 - 2 = 0 на [0, 2]: %lf\n", root);
    }
    if (dichotomy(0.0, 1.0, 1e-6, f_trig, &root) == STATUS_OK)
    {
        printf("Корень cos(x) - x = 0 на [0, 1]: %lf\n", root);
    }

    return 0;
}