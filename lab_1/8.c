#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_BUF_SIZE 1024

typedef enum {
    STATUS_OK,
    ARGC_ERROR,
    FILE_ERROR,
    PARSE_ERROR,
    RANGE_ERROR,
    STATUS_ERROR_FILE_OPEN
} status_code_t;

static int same_name(const char *p1, const char *p2)
{
    const char *n1 = strrchr(p1, '/');
    const char *n2 = strrchr(p2, '/');
    n1 = n1 ? n1 + 1 : p1;
    n2 = n2 ? n2 + 1 : p2;
    return strcmp(n1, n2) == 0;
}

static status_code_t check_same_files(const char *path1, const char *path2, int *out_same)
{
    if (path1 == NULL || path2 == NULL || out_same == NULL)
    {
        return FILE_ERROR;
    }
    
    char *rp1 = realpath(path1, NULL);
    if (rp1 == NULL)
    {
        return STATUS_ERROR_FILE_OPEN;
    }

    char *rp2 = realpath(path2, NULL);
    if (rp2 != NULL)
    {
        *out_same = (strcmp(rp1, rp2) == 0) ? 1 : 0;
        free(rp2);
    }
    else
    {
        *out_same = (strcmp(path1, path2) == 0) ? 1 : 0;
    }
    
    free(rp1);
    return STATUS_OK;
}

static int char_to_digit(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 10;
    } if (c >= 'a' && c <= 'z') {
        return c - 'a' + 10;
    }
    return -1;
}

static int is_space(int c) {
    return c == ' ' || c == '\t' || c == '\n';
}

static status_code_t read_lexeme(FILE *in, char *buf, int buf_size, int *out_len) {
    if (in == NULL || buf == NULL || out_len == NULL) {
        return FILE_ERROR;
    } if (buf_size < 2) {
        return RANGE_ERROR;
    }
    *out_len = 0;
    int c;
    while ((c = fgetc(in)) != EOF && is_space(c)) {
    }
    if (c == EOF) {
        return STATUS_OK;
    }
    int len = 0;
    buf[len++] = (char)c;
    while ((c = fgetc(in)) != EOF && !is_space(c)) {
        if (len + 1 >= buf_size) {
            return RANGE_ERROR;
        }
        buf[len++] = (char)c;
    }
    buf[len] = '\0';
    *out_len = len;
    return STATUS_OK;
}

static void skip_zeros(char *s) {
    int i = 0;
    while (s[i] == '0') {
        i++;
    }
    if (s[i] == '\0') {
        s[0] = '0';
        s[1] = '\0';
        return;
    }
    if (i == 0) {
        return;
    }
    int j = 0;
    while (s[i] != '\0') {
        s[j++] = s[i++];
    }
    s[j] = '\0';
}

static status_code_t process_number(const char *s, int *out_base, long long *out_value) {
    if (s == NULL || out_base == NULL || out_value == NULL) {
        return PARSE_ERROR;
    } if (*s == '\0') {
        return PARSE_ERROR;
    }
    int i = 0;
    int negative = 1;
    if (s[i] == '-') {
        negative = -1;
        i++;
    } if (s[i] == '\0') {
        return PARSE_ERROR;
    }
    int max_digit = 0;
    for (int j = i; s[j] != '\0'; ++j) {
        int d = char_to_digit(s[j]);
        if (d < 0) {
            return PARSE_ERROR;
        }
        if (d > max_digit) {
            max_digit = d;
        }
    }
    int base = max_digit + 1;
    if (base < 2) {
        base = 2;
    }
    if (base > 36) {
        return RANGE_ERROR;
    }
    long long result = 0;
    for (int j = i; s[j] != '\0'; ++j) {
        int d = char_to_digit(s[j]);
        if (result > (LLONG_MAX - (long long)d) / (long long)base) {
            return RANGE_ERROR;
        }
        result = result * (long long)base + (long long)d;
    }
    if (negative == -1) {
        result = -result;
    }
    *out_base = base;
    *out_value = result;
    return STATUS_OK;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Используется %s <входной файл> <выходной файл>\n", argv[0]);
        return ARGC_ERROR;
    }
    int same = 0;
    status_code_t err_check = check_same_files(argv[1], argv[2], &same);
    if (err_check != STATUS_OK) {
        printf("Не удалось проверить файлы '%s' или '%s'\n", argv[1], argv[2]);
        return FILE_ERROR;
    }
    if (same || same_name(argv[1], argv[2])) {
        printf("Входной и выходной файлы не могут совпадать\n");
        return FILE_ERROR;
    }

    FILE *in = fopen(argv[1], "r");
    if (in == NULL) {
        printf("Не удалось открыть '%s'\n", argv[1]);
        return FILE_ERROR;
    }
    FILE *out = fopen(argv[2], "w");
    if (out == NULL) {
        printf("Не удалось создать '%s'\n", argv[2]);
        fclose(in);
        return FILE_ERROR;
    }
    char buf[MAX_BUF_SIZE];
    while (1) {
        int len = 0;
        status_code_t err = read_lexeme(in, buf, MAX_BUF_SIZE, &len);
        if (err != STATUS_OK) {
            printf("Ошибка чтения\n");
            fclose(in);
            fclose(out);
            return err;
        }
        if (len == 0) {
            break;
        }
        skip_zeros(buf);
        int base = 0;
        long long value = 0;
        err = process_number(buf, &base, &value);
        if (err != STATUS_OK) {
            printf("Некорректное число '%s'\n", buf);
            fclose(in);
            fclose(out);
            return err;
        }
        fprintf(out, "%s %d %lld\n", buf, base, value);
    }
    fclose(in);
    fclose(out);
    return STATUS_OK;
}