#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

typedef enum {
    OK = 0,
    INVALID_INPUT,
    INVALID_MEMORY,
    ERR_ARGC,
    INVALID_DIM,
    INVALID_NORM
} status;

#define EPS 1e-9

typedef struct {
    double *coords;
    size_t n;
} Vector;

typedef double (*NormFunc)(const Vector *v, void *ctx);

typedef struct {
    size_t *indices;
    size_t count;
    double value;
} NormResult;

static status vector_create(size_t n, Vector *out)
{
    if (out == NULL || n == 0) return INVALID_INPUT;
    out->coords = calloc(n, sizeof(double));
    if (!out->coords) return INVALID_MEMORY;
    out->n = n;
    return OK;
}

static void vector_free(Vector *v)
{
    if (v == NULL) return;
    free(v->coords);
    v->coords = NULL;
    v->n = 0;
}

static double norm_inf(const Vector *v, void *ctx)
{
    (void)ctx;
    double m = 0.0;
    for (size_t i = 0; i < v->n; i++) {
        double a = fabs(v->coords[i]);
        if (a > m) m = a;
    }
    return m;
}

typedef struct {
    double p;
} PNormCtx;

static double norm_p(const Vector *v, void *ctx)
{
    PNormCtx *c = ctx;
    double p = c->p;
    double sum = 0.0;
    for (size_t i = 0; i < v->n; i++)
        sum += pow(fabs(v->coords[i]), p);
    return pow(sum, 1.0 / p);
}

typedef struct {
    double *A;
    size_t n;
} ANormCtx;

static double norm_A(const Vector *v, void *ctx)
{
    ANormCtx *c = ctx;
    size_t n = v->n;
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) {
        double row = 0.0;
        for (size_t j = 0; j < n; j++)
            row += c->A[i * n + j] * v->coords[j];
        sum += row * v->coords[i];
    }
    return sqrt(sum > 0.0 ? sum : 0.0);
}

static status find_longest_vectors(size_t n, size_t norm_count,
                                   size_t vector_count,
                                   NormResult **out_results,
                                   ...)
{
    if (out_results == NULL || n == 0 || norm_count == 0 || vector_count == 0)
        return INVALID_INPUT;

    va_list ap;
    va_start(ap, out_results);

    NormFunc *norms = calloc(norm_count, sizeof(NormFunc));
    void **ctxs = calloc(norm_count, sizeof(void *));
    Vector *vecs = calloc(vector_count, sizeof(Vector));

    if (!norms || !ctxs || !vecs) {
        free(norms); free(ctxs); free(vecs);
        va_end(ap);
        return INVALID_MEMORY;
    }

    for (size_t k = 0; k < norm_count; k++) {
        norms[k] = va_arg(ap, NormFunc);
        ctxs[k] = va_arg(ap, void *);
        if (norms[k] == NULL) {
            free(norms); free(ctxs); free(vecs);
            va_end(ap);
            return INVALID_NORM;
        }
    }

    for (size_t k = 0; k < vector_count; k++) {
        Vector *v = va_arg(ap, Vector *);
        if (v == NULL || v->n != n) {
            for (size_t j = 0; j < k; j++) vector_free(&vecs[j]);
            free(norms); free(ctxs); free(vecs);
            va_end(ap);
            return INVALID_DIM;
        }
        status st = vector_create(n, &vecs[k]);
        if (st != OK) {
            for (size_t j = 0; j < k; j++) vector_free(&vecs[j]);
            free(norms); free(ctxs); free(vecs);
            va_end(ap);
            return st;
        }
        for (size_t i = 0; i < n; i++)
            vecs[k].coords[i] = v->coords[i];
    }

    va_end(ap);

    NormResult *results = calloc(norm_count, sizeof(NormResult));
    if (!results) {
        for (size_t j = 0; j < vector_count; j++) vector_free(&vecs[j]);
        free(norms); free(ctxs); free(vecs);
        return INVALID_MEMORY;
    }

    for (size_t k = 0; k < norm_count; k++) {
        double *vals = malloc(vector_count * sizeof(double));
        if (!vals) {
            for (size_t j = 0; j < k; j++) free(results[j].indices);
            free(results);
            for (size_t j = 0; j < vector_count; j++) vector_free(&vecs[j]);
            free(norms); free(ctxs); free(vecs);
            return INVALID_MEMORY;
        }
        for (size_t i = 0; i < vector_count; i++)
            vals[i] = norms[k](&vecs[i], ctxs[k]);

        double best = vals[0];
        for (size_t i = 1; i < vector_count; i++)
            if (vals[i] > best) best = vals[i];

        size_t cnt = 0;
        for (size_t i = 0; i < vector_count; i++)
            if (fabs(vals[i] - best) <= EPS) cnt++;

        results[k].indices = malloc(cnt * sizeof(size_t));
        if (!results[k].indices) {
            free(vals);
            for (size_t j = 0; j < k; j++) free(results[j].indices);
            free(results);
            for (size_t j = 0; j < vector_count; j++) vector_free(&vecs[j]);
            free(norms); free(ctxs); free(vecs);
            return INVALID_MEMORY;
        }
        size_t pos = 0;
        for (size_t i = 0; i < vector_count; i++)
            if (fabs(vals[i] - best) <= EPS) results[k].indices[pos++] = i;
        results[k].count = cnt;
        results[k].value = best;
        free(vals);
    }

    for (size_t j = 0; j < vector_count; j++) vector_free(&vecs[j]);
    free(norms); free(ctxs); free(vecs);

    *out_results = results;
    return OK;
}

static void free_results(NormResult *results, size_t norm_count)
{
    if (results == NULL) return;
    for (size_t k = 0; k < norm_count; k++) free(results[k].indices);
    free(results);
}

int main(void)
{
    size_t n, k;
    if (scanf("%zu %zu", &n, &k) != 2 || n == 0 || k == 0) return INVALID_INPUT;

    Vector *vecs = calloc(k, sizeof(Vector));
    if (!vecs) return INVALID_MEMORY;

    for (size_t i = 0; i < k; i++) {
        if (vector_create(n, &vecs[i]) != OK) {
            for (size_t j = 0; j < i; j++) vector_free(&vecs[j]);
            free(vecs);
            return INVALID_MEMORY;
        }
        for (size_t j = 0; j < n; j++)
            if (scanf("%lf", &vecs[i].coords[j]) != 1) {
                for (size_t q = 0; q <= i; q++) vector_free(&vecs[q]);
                free(vecs);
                return INVALID_INPUT;
            }
    }

    size_t m;
    if (scanf("%zu", &m) != 1 || m == 0) {
        for (size_t i = 0; i < k; i++) vector_free(&vecs[i]);
        free(vecs);
        return INVALID_INPUT;
    }

    NormFunc *norms = calloc(m, sizeof(NormFunc));
    void **ctxs = calloc(m, sizeof(void *));
    const char **names = calloc(m, sizeof(char *));
    if (!norms || !ctxs || !names) {
        free(norms); free(ctxs); free(names);
        for (size_t i = 0; i < k; i++) vector_free(&vecs[i]);
        free(vecs);
        return INVALID_MEMORY;
    }

    PNormCtx *pctx = NULL;
    ANormCtx *actx = NULL;
    double *Amat = NULL;
    status st = OK;

    for (size_t i = 0; i < m; i++) {
        int type;
        if (scanf("%d", &type) != 1) { st = INVALID_INPUT; break; }
        if (type == 1) {
            norms[i] = norm_inf; ctxs[i] = NULL; names[i] = "inf";
        } else if (type == 2) {
            pctx = malloc(sizeof(PNormCtx));
            if (!pctx || scanf("%lf", &pctx->p) != 1 || pctx->p < 1.0) {
                free(pctx); pctx = NULL; st = INVALID_INPUT; break;
            }
            norms[i] = norm_p; ctxs[i] = pctx; names[i] = "p";
        } else if (type == 3) {
            actx = malloc(sizeof(ANormCtx));
            Amat = malloc(n * n * sizeof(double));
            if (!actx || !Amat) { free(actx); free(Amat); st = INVALID_MEMORY; break; }
            actx->A = Amat; actx->n = n;
            int ok = 1;
            for (size_t r = 0; r < n && ok; r++)
                for (size_t c = 0; c < n; c++)
                    if (scanf("%lf", &Amat[r * n + c]) != 1) { ok = 0; break; }
            if (!ok) { st = INVALID_INPUT; break; }
            norms[i] = norm_A; ctxs[i] = actx; names[i] = "A";
        } else { st = INVALID_INPUT; break; }
    }

    if (st == OK) {
        for (size_t i = 0; i < m; i++) {
            double best = -1.0;
            size_t count = 0;
            for (size_t j = 0; j < k; j++) {
                double v = norms[i](&vecs[j], ctxs[i]);
                if (v > best + EPS) { best = v; count = 1; }
                else if (fabs(v - best) <= EPS) count++;
            }
            printf("%s: ", names[i]);
            for (size_t j = 0; j < k; j++) {
                double v = norms[i](&vecs[j], ctxs[i]);
                if (fabs(v - best) <= EPS) printf("%zu ", j);
            }
            printf("\n");
        }
    }

    free(pctx); free(actx); free(Amat);
    free(norms); free(ctxs); free(names);
    for (size_t i = 0; i < k; i++) vector_free(&vecs[i]);
    free(vecs);

    return st;
}
int main(void)
{
    size_t n, k;
    printf("Введите размерность векторов (n) и количество векторов (k): ");
    if (scanf("%zu %zu", &n, &k) != 2 || n == 0 || k == 0) {
        printf("Ошибка: неверные входные данные для размерности или количества.\n");
        return INVALID_INPUT;
    }

    Vector *vecs = calloc(k, sizeof(Vector));
    if (!vecs) {
        printf("Ошибка выделения памяти.\n");
        return INVALID_MEMORY;
    }

    printf("Введите координаты для каждого из %zu векторов (по %zu чисел через пробел):\n", k, n);
    for (size_t i = 0; i < k; i++) {
        if (vector_create(n, &vecs[i]) != OK) {
            for (size_t j = 0; j < i; j++) vector_free(&vecs[j]);
            free(vecs);
            return INVALID_MEMORY;
        }
        printf("Вектор %zu: ", i + 1);
        for (size_t j = 0; j < n; j++) {
            if (scanf("%lf", &vecs[i].coords[j]) != 1) {
                printf("Ошибка чтения координаты вектора.\n");
                for (size_t q = 0; q <= i; q++) vector_free(&vecs[q]);
                free(vecs);
                return INVALID_INPUT;
            }
        }
    }

    size_t m;
    printf("Введите количество норм (m): ");
    if (scanf("%zu", &m) != 1 || m == 0) {
        printf("Ошибка: неверное количество норм.\n");
        for (size_t i = 0; i < k; i++) vector_free(&vecs[i]);
        free(vecs);
        return INVALID_INPUT;
    }

    NormFunc *norms = calloc(m, sizeof(NormFunc));
    void **ctxs = calloc(m, sizeof(void *));
    const char **names = calloc(m, sizeof(char *));
    if (!norms || !ctxs || !names) {
        printf("Ошибка выделения памяти.\n");
        free(norms); free(ctxs); free(names);
        for (size_t i = 0; i < k; i++) vector_free(&vecs[i]);
        free(vecs);
        return INVALID_MEMORY;
    }

    PNormCtx *pctx = NULL;
    ANormCtx *actx = NULL;
    double *Amat = NULL;
    status st = OK;

    printf("\nТипы норм:\n  1 - Infinity-норма\n  2 - p-норма (требуется ввести p >= 1.0)\n  3 - Матричная норма A (требуется ввести матрицу %zux%zu)\n", n, n);

    for (size_t i = 0; i < m; i++) {
        int type;
        printf("Введите тип для нормы %zu: ", i + 1);
        if (scanf("%d", &type) != 1) { 
            st = INVALID_INPUT; 
            break; 
        }

        if (type == 1) {
            norms[i] = norm_inf; 
            ctxs[i] = NULL; 
            names[i] = "inf";
        } else if (type == 2) {
            pctx = malloc(sizeof(PNormCtx));
            printf("Введите параметр p (p >= 1.0): ");
            if (!pctx || scanf("%lf", &pctx->p) != 1 || pctx->p < 1.0) {
                printf("Ошибка: неверное значение p.\n");
                free(pctx); 
                pctx = NULL; 
                st = INVALID_INPUT; 
                break;
            }
            norms[i] = norm_p; 
            ctxs[i] = pctx; 
            names[i] = "p";
        } else if (type == 3) {
            actx = malloc(sizeof(ANormCtx));
            Amat = malloc(n * n * sizeof(double));
            if (!actx || !Amat) { 
                free(actx); 
                free(Amat); 
                st = INVALID_MEMORY; 
                break; 
            }
            actx->A = Amat; 
            actx->n = n;
            
            printf("Введите элементы матрицы A (%zux%zu):\n", n, n);
            int ok = 1;
            for (size_t r = 0; r < n && ok; r++) {
                for (size_t c = 0; c < n; c++) {
                    if (scanf("%lf", &Amat[r * n + c]) != 1) { 
                        ok = 0; 
                        break; 
                    }
                }
            }
            if (!ok) { 
                printf("Ошибка чтения матрицы.\n");
                st = INVALID_INPUT; 
                break; 
            }
            norms[i] = norm_A; 
            ctxs[i] = actx; 
            names[i] = "A";
        } else {
            printf("Ошибка: неизвестный тип нормы.\n");
            st = INVALID_INPUT; 
            break; 
        }
    }

    if (st == OK) {
        printf("\n--- Результаты ---\n");
        for (size_t i = 0; i < m; i++) {
            double best = -1.0;
            size_t count = 0;
            for (size_t j = 0; j < k; j++) {
                double v = norms[i](&vecs[j], ctxs[i]);
                if (v > best + EPS) { 
                    best = v; 
                    count = 1; 
                } else if (fabs(v - best) <= EPS) {
                    count++;
                }
            }
            printf("%s: ", names[i]);
            for (size_t j = 0; j < k; j++) {
                double v = norms[i](&vecs[j], ctxs[i]);
                if (fabs(v - best) <= EPS) {
                    printf("%zu ", j);
                }
            }
            printf("\n");
        }
    }

    free(pctx); 
    free(actx); 
    free(Amat);
    free(norms); 
    free(ctxs); 
    free(names);
    for (size_t i = 0; i < k; i++) vector_free(&vecs[i]);
    free(vecs);

    return st;
}