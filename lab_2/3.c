#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    STATUS_OK,
    STATUS_ERROR_NULL_POINTER,
    STATUS_ERROR_OUT_OF_MEMORY,
    STATUS_ERROR_INVALID_ARGUMENT,
    STATUS_ERROR_FILE_IO,
    STATUS_ERROR_OVERFLOW,
    STATUS_ERROR_EMPTY_FILE
} status_code_t;

static status_code_t read_file_to_buffer(const char *filepath, char **out_buf, size_t *out_size)
{
    if (!filepath || !out_buf || !out_size)
        return STATUS_ERROR_NULL_POINTER;

    FILE *file = fopen(filepath, "r");
    if (!file)
        return STATUS_ERROR_FILE_IO;

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return STATUS_ERROR_FILE_IO;
    }

    long size = ftell(file);
    if (size < 0)
    {
        fclose(file);
        return STATUS_ERROR_FILE_IO;
    }

    if ((unsigned long)size >= SIZE_MAX)
    {
        fclose(file);
        return STATUS_ERROR_OVERFLOW;
    }

    rewind(file);

    char *buf = (char *)malloc((size_t)size + 1);
    if (!buf)
    {
        fclose(file);
        return STATUS_ERROR_OUT_OF_MEMORY;
    }

    size_t read_bytes = fread(buf, 1, (size_t)size, file);
    buf[read_bytes] = '\0';
    fclose(file);

    *out_buf = buf;
    *out_size = read_bytes;
    return STATUS_OK;
}

static void get_line_and_column(const char *buf, size_t index, size_t *out_line, size_t *out_col)
{
    size_t line = 1;
    size_t col = 1;
    for (size_t i = 0; i < index; ++i)
    {
        if (buf[i] == '\n')
        {
            line++;
            col = 1;
        }
        else
        {
            col++;
        }
    }
    *out_line = line;
    *out_col = col;
}

static status_code_t find_all_occurrences(const char *text, size_t text_len, const char *pattern, size_t pat_len, size_t **out_indices, size_t *out_count)
{
    if (!text || !pattern || !out_indices || !out_count)
        return STATUS_ERROR_NULL_POINTER;

    if (pat_len == 0 || pat_len > text_len)
    {
        *out_indices = NULL;
        *out_count = 0;
        return STATUS_OK;
    }

    int bad_char[256];
    for (int i = 0; i < 256; ++i)
    {
        bad_char[i] = (int)pat_len;
    }
    for (size_t i = 0; i < pat_len - 1; ++i)
    {
        bad_char[(unsigned char)pattern[i]] = (int)(pat_len - i - 1);
    }

    size_t cap = 8;
    size_t count = 0;
    size_t *indices = (size_t *)malloc(cap * sizeof(size_t));
    if (!indices)
        return STATUS_ERROR_OUT_OF_MEMORY;

    size_t i = 0;
    while (i <= text_len - pat_len)
    {
        bool match = true;
        for (size_t j = 0; j < pat_len; ++j)
        {
            if (text[i + j] != pattern[j])
            {
                match = false;
                break;
            }
        }

        if (match)
        {
            if (count >= cap)
            {
                cap *= 2;
                size_t *new_indices = (size_t *)realloc(indices, cap * sizeof(size_t));
                if (!new_indices)
                {
                    free(indices);
                    return STATUS_ERROR_OUT_OF_MEMORY;
                }
                indices = new_indices;
            }
            indices[count++] = i;
        }

        if (i + pat_len >= text_len)
            break;
        i += bad_char[(unsigned char)text[i + pat_len - 1]];
    }

    *out_indices = indices;
    *out_count = count;
    return STATUS_OK;
}

static status_code_t search_in_single_file(const char *substring, const char *filepath)
{
    if (!substring || !filepath)
        return STATUS_ERROR_NULL_POINTER;

    size_t pat_len = strlen(substring);
    char *file_buf = NULL;
    size_t file_size = 0;
    status_code_t sc = read_file_to_buffer(filepath, &file_buf, &file_size);
    if (sc != STATUS_OK)
        return sc;
    if (file_size == 0)
    {
        free(file_buf);
        return STATUS_ERROR_EMPTY_FILE;
    }

    size_t *indices = NULL;
    size_t count = 0;
    sc = find_all_occurrences(file_buf, file_size, substring, pat_len, &indices, &count);
    if (sc != STATUS_OK)
    {
        free(file_buf);
        return sc;
    }

    printf("Файл: \"%s\"\n", filepath);
    if (count == 0)
    {
        printf("  Подстрока не найдена.\n");
    }
    else
    {
        printf("  Найдено вхождений: %zu:\n", count);
        for (size_t i = 0; i < count; ++i)
        {
            size_t line = 0;
            size_t col = 0;
            get_line_and_column(file_buf, indices[i], &line, &col);
            printf("    [%zu] Строка: %zu, Столбец: %zu\n", i + 1, line, col);
        }
    }
    printf("\n");

    free(indices);
    free(file_buf);
    return STATUS_OK;
}

static status_code_t search_substring_in_files(const char *substring, int file_count, ...)
{
    if (!substring)
        return STATUS_ERROR_NULL_POINTER;
    if (file_count <= 0)
        return STATUS_ERROR_INVALID_ARGUMENT;

    va_list args;
    va_start(args, file_count);

    for (int f_idx = 0; f_idx < file_count; ++f_idx)
    {
        const char *filepath = va_arg(args, const char *);
        if (!filepath)
        {
            va_end(args);
            return STATUS_ERROR_NULL_POINTER;
        }
        status_code_t sc = search_in_single_file(substring, filepath);
        if (sc != STATUS_OK)
        {
            va_end(args);
            return sc;
        }
    }

    va_end(args);
    return STATUS_OK;
}

static const char *status_to_string(status_code_t sc)
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
    case STATUS_ERROR_FILE_IO:
        return "Ошибка ввода-вывода файла (не удается открыть или прочитать файл)";
    case STATUS_ERROR_OVERFLOW:
        return "Файл слишком большой (переполнение размера)";
    case STATUS_ERROR_EMPTY_FILE:
        return "Файл пуст";
    default:
        return "Неизвестная ошибка";
    }
}

static void print_error(status_code_t sc, const char *filepath)
{
    if (filepath)
    {
        printf("Ошибка: %s (файл: '%s', код: %d)\n",
               status_to_string(sc), filepath, (int)sc);
    }
    else
    {
        printf("Ошибка: %s (код: %d)\n",
               status_to_string(sc), (int)sc);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Использование: %s <подстрока> <файл1> [<файл2> ...]\n", argv[0]);
        return STATUS_ERROR_INVALID_ARGUMENT;
    }

    const char *substring = argv[1];
    int file_count = argc - 2;
    status_code_t final_sc = STATUS_OK;

    for (int i = 0; i < file_count; ++i)
    {
        const char *filepath = argv[2 + i];
        status_code_t sc = search_in_single_file(substring, filepath);
        if (sc != STATUS_OK)
        {
            print_error(sc, filepath);
            final_sc = sc;
        }
    }

    return final_sc;
}