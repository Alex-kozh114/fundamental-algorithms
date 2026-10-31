#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

typedef enum
{
    OK,
    INVALID_INPUT,
    INVALID_MEMORY,
    ERR_ARGC
} status;

typedef struct
{
    char *filename;
    unsigned int size;
    unsigned int importance;
} FileDesc;

typedef struct
{
    FileDesc *files;
    size_t count;
    unsigned long long total_size;
    unsigned long long total_importance;
} BackupPlan;

static status validate_uint(const char *s, unsigned int *out)
{
    if (s == NULL || out == NULL || *s == '\0')
        return INVALID_INPUT;
    unsigned long long v = 0;
    for (size_t i = 0; s[i] != '\0'; i++)
    {
        if (s[i] < '0' || s[i] > '9')
            return INVALID_INPUT;
        v = v * 10ULL + (unsigned)(s[i] - '0');
        if (v > UINT_MAX)
            return INVALID_INPUT;
    }
    *out = (unsigned int)v;
    return OK;
}

static status validate_positive_uint(const char *s, unsigned int *out)
{
    status st = validate_uint(s, out);
    if (st != OK)
        return st;
    if (*out == 0)
        return INVALID_INPUT;
    return OK;
}

static status validate_filename(const char *s)
{
    if (s == NULL || *s == '\0')
        return INVALID_INPUT;
    for (size_t i = 0; s[i] != '\0'; i++)
    {
        char c = s[i];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            return INVALID_INPUT;
    }
    return OK;
}

static char *dup_string(const char *s)
{
    size_t len = 0;
    while (s[len] != '\0')
        len++;
    char *res = malloc(len + 1);
    if (!res)
        return NULL;
    for (size_t i = 0; i <= len; i++)
        res[i] = s[i];
    return res;
}

static status read_input(FileDesc **out_files, size_t *out_n,
                         unsigned int *out_capacity)
{
    unsigned int n_u, cap;
    if (scanf("%u %u", &n_u, &cap) != 2)
        return INVALID_INPUT;
    if (n_u == 0)
        return INVALID_INPUT;

    FileDesc *files = calloc(n_u, sizeof(FileDesc));
    if (!files)
        return INVALID_MEMORY;

    for (unsigned int i = 0; i < n_u; i++)
    {
        char fname[256];
        unsigned int size, imp;
        if (scanf("%255s %u %u", fname, &size, &imp) != 3)
        {
            for (unsigned int k = 0; k < i; k++)
                free(files[k].filename);
            free(files);
            return INVALID_INPUT;
        }
        if (validate_filename(fname) != OK || size == 0)
        {
            for (unsigned int k = 0; k < i; k++)
                free(files[k].filename);
            free(files);
            return INVALID_INPUT;
        }
        char *copy = dup_string(fname);
        if (!copy)
        {
            for (unsigned int k = 0; k < i; k++)
                free(files[k].filename);
            free(files);
            return INVALID_MEMORY;
        }
        files[i].filename = copy;
        files[i].size = size;
        files[i].importance = imp;
    }

    *out_files = files;
    *out_n = n_u;
    *out_capacity = cap;
    return OK;
}

static status knapsack(const FileDesc *files, size_t n,
                       unsigned int capacity, BackupPlan *out)
{
    if (files == NULL || out == NULL)
        return INVALID_INPUT;

    size_t w = (size_t)capacity + 1;
    unsigned long long *dp = calloc((n + 1) * w, sizeof(unsigned long long));
    if (!dp)
        return INVALID_MEMORY;

    for (size_t i = 1; i <= n; i++)
    {
        unsigned int sz = files[i - 1].size;
        unsigned int imp = files[i - 1].importance;
        for (size_t c = 0; c <= capacity; c++)
        {
            unsigned long long best = dp[(i - 1) * w + c];
            if (sz <= c)
            {
                unsigned long long cand = dp[(i - 1) * w + (c - sz)] + imp;
                if (cand > best)
                    best = cand;
            }
            dp[i * w + c] = best;
        }
    }

    size_t chosen = 0;
    for (size_t i = n; i > 0; i--)
    {
        if (dp[i * w + capacity] != dp[(i - 1) * w + capacity])
            chosen++;
        else
        {
            unsigned int sz = files[i - 1].size;
            unsigned int imp = files[i - 1].importance;
            if (sz <= capacity)
            {
                if (dp[(i - 1) * w + (capacity - sz)] + imp ==
                    dp[i * w + capacity])
                    chosen++;
            }
        }
    }

    FileDesc *result = NULL;
    if (chosen > 0)
    {
        result = calloc(chosen, sizeof(FileDesc));
        if (!result)
        {
            free(dp);
            return INVALID_MEMORY;
        }
    }

    size_t idx = chosen;
    size_t c = capacity;
    for (size_t i = n; i > 0; i--)
    {
        unsigned long long cur = dp[i * w + c];
        unsigned long long prev = dp[(i - 1) * w + c];
        if (cur == prev)
            continue;
        unsigned int sz = files[i - 1].size;
        if (sz > c)
            continue;
        result[--idx] = files[i - 1];
        c -= sz;
    }

    unsigned long long tot_size = 0, tot_imp = 0;
    for (size_t i = 0; i < chosen; i++)
    {
        tot_size += result[i].size;
        tot_imp += result[i].importance;
    }

    out->files = result;
    out->count = chosen;
    out->total_size = tot_size;
    out->total_importance = tot_imp;

    free(dp);
    return OK;
}

static void print_plan(const BackupPlan *plan)
{
    printf("TOTAL_SIZE %llu\n", plan->total_size);
    printf("TOTAL_IMPORTANCE %llu\n", plan->total_importance);
    printf("FILES %zu\n", plan->count);
    for (size_t i = 0; i < plan->count; i++)
        printf("%s\n", plan->files[i].filename);
}

static void free_files(FileDesc *files, size_t n)
{
    if (files == NULL)
        return;
    for (size_t i = 0; i < n; i++)
        free(files[i].filename);
    free(files);
}

static void print_status(status st)
{
    switch (st)
    {
    case INVALID_INPUT:
        printf("ошибка неверные входные данные\n");
        break;
    case INVALID_MEMORY:
        printf("ошибка не удалось выделить память\n");
        break;
    case ERR_ARGC:
        printf("обишка неверное кол-во аргументовы\n");
        break;
    default:
        break;
    }
}

int main(void)
{
    FileDesc *files = NULL;
    size_t n = 0;
    unsigned int capacity = 0;

    status st = read_input(&files, &n, &capacity);
    if (st != OK)
    {
        print_status(st);
        return st;
    }

    BackupPlan plan;
    st = knapsack(files, n, capacity, &plan);
    if (st != OK)
    {
        print_status(st);
        free_files(files, n);
        return st;
    }

    print_plan(&plan);

    free(plan.files);
    free_files(files, n);
    return OK;
}