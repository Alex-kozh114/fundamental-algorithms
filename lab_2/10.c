#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

typedef enum
{
    OK = 0,
    INVALID_INPUT,
    INVALID_MEMORY,
    INVALID_DEGREE,
    INVALID_EPS
} status;

#define EPS_DEFAULT 1e-9

static status horner_shift(const double *f, int n, double a, double **out_g)
{
    if (f == NULL || out_g == NULL)
        return INVALID_INPUT;
    if (n < 0)
        return INVALID_DEGREE;

    double *g = (double *)malloc((size_t)(n + 1) * sizeof(double));
    if (!g)
        return INVALID_MEMORY;

    double *b = (double *)malloc((size_t)(n + 1) * sizeof(double));
    if (!b)
    {
        free(g);
        return INVALID_MEMORY;
    }

    for (int i = 0; i <= n; i++)
        b[i] = f[i];

    for (int k = 0; k <= n; k++) {
    int m = n - k;
    double prev = b[m];
    for (int i = m - 1; i >= 0; i--) {
        double cur = b[i];
        b[i] = prev;
        prev = cur + a * prev;
    }
    g[k] = prev;
}

    free(b);
    *out_g = g;
    return OK;
}

static status shift_polynomial(double eps, double a, double **out_g,
                               int n, ...)
{
    if (out_g == NULL)
        return INVALID_INPUT;
    if (n < 0)
        return INVALID_DEGREE;
    if (eps <= 0.0)
        return INVALID_EPS;

    double *f = (double *)malloc((size_t)(n + 1) * sizeof(double));
    if (!f)
        return INVALID_MEMORY;

    va_list ap;
    va_start(ap, n);
    for (int i = 0; i <= n; i++)
        f[i] = va_arg(ap, double);
    va_end(ap);

    status st = horner_shift(f, n, a, out_g);
    free(f);
    if (st != OK)
        return st;

    for (int i = 0; i <= n; i++)
        if (fabs((*out_g)[i]) < eps)
            (*out_g)[i] = 0.0;

    return OK;
}

static void print_poly(const double *g, int n, double a)
{
    printf("g(x) = ");
    int first = 1;
    for (int i = n; i >= 0; i--)
    {
        if (g[i] == 0.0)
            continue;
        if (!first)
            printf(" + ");
        first = 0;
        if (i == 0)
            printf("%.6g", g[i]);
        else if (i == 1)
            printf("%.6g*(x - %.6g)", g[i], a);
        else
            printf("%.6g*(x - %.6g)^%d", g[i], a, i);
    }
    if (first)
        printf("0");
    printf("\n");
}

static void print_status(status st)
{
    switch (st)
    {
    case INVALID_INPUT:
        printf("Error: invalid input\n");
        break;
    case INVALID_MEMORY:
        printf("Error: memory allocation failed\n");
        break;
    case INVALID_DEGREE:
        printf("Error: degree must be >= 0\n");
        break;
    case INVALID_EPS:
        printf("Error: eps must be > 0\n");
        break;
    default:
        break;
    }
}

int main(void)
{
    status st;
    double *g = NULL;

    printf("f(x) = x^3, a = 1\n");
    st = shift_polynomial(EPS_DEFAULT, 1.0, &g, 3, 0.0, 0.0, 0.0, 1.0);
    if (st == OK)
    {
        print_poly(g, 3, 1.0);
        free(g);
    }
    else
        print_status(st);

    printf("\nf(x) = x^2, a = 1\n");
    st = shift_polynomial(EPS_DEFAULT, 1.0, &g, 2, 0.0, 0.0, 1.0);
    if (st == OK)
    {
        print_poly(g, 2, 1.0);
        free(g);
    }
    else
        print_status(st);

    printf("\nf(x) = 2 + 3x + 4x^2, a = 2\n");
    st = shift_polynomial(EPS_DEFAULT, 2.0, &g, 2, 2.0, 3.0, 4.0);
    if (st == OK)
    {
        print_poly(g, 2, 2.0);
        free(g);
    }
    else
        print_status(st);

    printf("\nf(x) = 5 (constant), a = 3\n");
    st = shift_polynomial(EPS_DEFAULT, 3.0, &g, 0, 5.0);
    if (st == OK)
    {
        print_poly(g, 0, 3.0);
        free(g);
    }
    else
        print_status(st);

    printf("\ninvalid: eps <= 0\n");
    st = shift_polynomial(0.0, 1.0, &g, 2, 1.0, 2.0, 3.0);
    if (st != OK)
        print_status(st);

    printf("\ninvalid: negative degree\n");
    st = shift_polynomial(EPS_DEFAULT, 1.0, &g, -1);
    if (st != OK)
        print_status(st);

    return OK;
}
