#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef enum
{
    STATUS_OK,
    STATUS_ERROR_INVALID_ARG_COUNT,
    STATUS_ERROR_INVALID_OPTION,
    STATUS_ERROR_INVALID_ARG_VALUE,
    STATUS_ERROR_FILE_OPEN,
    STATUS_ERROR_FILE_READ,
    STATUS_ERROR_FILE_WRITE,
    STATUS_ERROR_MEMORY_ALLOCATION,
    STATUS_ERROR_NULL_POINTER
} status_code_t;

static void print_error_message(status_code_t code)
{
    switch (code)
    {
    case STATUS_ERROR_INVALID_ARG_COUNT:
        fprintf(stderr, "Ошибка: Неверное количество аргументов командной строки.\n");
        break;
    case STATUS_ERROR_INVALID_OPTION:
        fprintf(stderr, "Ошибка: Неизвестный или некорректный формат флага.\n");
        break;
    case STATUS_ERROR_INVALID_ARG_VALUE:
        fprintf(stderr, "Ошибка: Недопустимое значение аргумента.\n");
        break;
    case STATUS_ERROR_FILE_OPEN:
        fprintf(stderr, "Ошибка: Не удалось открыть или создать файл.\n");
        break;
    case STATUS_ERROR_FILE_READ:
        fprintf(stderr, "Ошибка при чтении из файла.\n");
        break;
    case STATUS_ERROR_FILE_WRITE:
        fprintf(stderr, "Ошибка при записи в файл.\n");
        break;
    case STATUS_ERROR_MEMORY_ALLOCATION:
        fprintf(stderr, "Ошибка: Не удалось выделить память.\n");
        break;
    case STATUS_ERROR_NULL_POINTER:
        fprintf(stderr, "Ошибка: В функцию был передан пустой указатель (NULL).\n");
        break;
    default:
        fprintf(stderr, "Произошла неизвестная ошибка (Код: %d).\n", code);
        break;
    }
}

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
        return STATUS_ERROR_NULL_POINTER;
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

static status_code_t parse_flag(const char *s, char *out_flag, bool *out_n)
{
    if (s == NULL || out_flag == NULL || out_n == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    size_t len = strlen(s);
    if (len != 2 && len != 3)
    {
        return STATUS_ERROR_INVALID_OPTION;
    }
    if (s[0] != '-' && s[0] != '/')
    {
        return STATUS_ERROR_INVALID_OPTION;
    }

    char c;
    if (len == 3)
    {
        if (s[1] != 'n')
        {
            return STATUS_ERROR_INVALID_OPTION;
        }
        c = s[2];
        *out_n = true;
    }
    else
    {
        c = s[1];
        *out_n = false;
    }

    if (c != 'd' && c != 'i' && c != 's' && c != 'a')
    {
        return STATUS_ERROR_INVALID_OPTION;
    }

    *out_flag = c;
    return STATUS_OK;
}

static status_code_t generate_output_filename(const char *input, char **out_name)
{
    if (input == NULL || out_name == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }

    const char *slash = strrchr(input, '/');
    int prefix_len = 0;
    const char *file_name = NULL;

    if (slash != NULL)
    {
        prefix_len = (int)(slash - input) + 1;
        file_name = slash + 1;
    }
    else
    {
        prefix_len = 0;
        file_name = input; 
    }

    size_t file_name_len = strlen(file_name);
    size_t total_len = prefix_len + 4 + file_name_len + 1;

    char *result = (char *)malloc(total_len * sizeof(char));
    if (result == NULL)
    {
        return STATUS_ERROR_MEMORY_ALLOCATION;
    }

    int i = 0;
    for (int j = 0; j < prefix_len; ++j)
    {
        result[i++] = input[j];
    }
    result[i++] = 'o';
    result[i++] = 'u';
    result[i++] = 't';
    result[i++] = '_';
    for (size_t j = 0; j < file_name_len; ++j)
    {
        result[i++] = file_name[j];
    }
    result[i] = '\0';

    *out_name = result;
    return STATUS_OK;
}

static status_code_t flag_d(FILE *in, FILE *out) {
    if (in == NULL || out == NULL) {
        return STATUS_ERROR_NULL_POINTER;
    }
    int c;
    while ((c = fgetc(in)) != EOF) {
        if (c >= '0' && c <= '9') {
            continue;
        }
        if (fputc(c, out) == EOF) {
            return STATUS_ERROR_FILE_WRITE;
        }
    }
    return STATUS_OK;
}

static status_code_t flag_i(FILE *in, FILE *out)
{
    if (in == NULL || out == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    char line[1024];
    while (fgets(line, sizeof(line), in) != NULL)
    {
        int count = 0;
        for (int i = 0; line[i] != '\0'; ++i)
        {
            char c = line[i];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
            {
                count++;
            }
        }
        if (fprintf(out, "%d\n", count) < 0)
        {
            return STATUS_ERROR_FILE_WRITE;
        }
    }
    return STATUS_OK;
}

static status_code_t flag_s(FILE *in, FILE *out)
{
    if (in == NULL || out == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    char line[1024];
    while (fgets(line, sizeof(line), in) != NULL)
    {
        int count = 0;
        for (int i = 0; line[i] != '\0' && line[i] != '\n'; ++i)
        {
            char c = line[i];
            bool is_letter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
            bool is_digit = (c >= '0' && c <= '9');
            bool is_space = (c == ' ');
            if (!is_letter && !is_digit && !is_space)
            {
                count++;
            }
        }
        if (fprintf(out, "%d\n", count) < 0)
        {
            return STATUS_ERROR_FILE_WRITE;
        }
    }
    return STATUS_OK;
}

static status_code_t flag_a(FILE *in, FILE *out)
{
    if (in == NULL || out == NULL)
    {
        return STATUS_ERROR_NULL_POINTER;
    }
    int c;
    while ((c = fgetc(in)) != EOF)
    {
        if (c >= '0' && c <= '9')
        {
            if (fputc(c, out) == EOF)
                return STATUS_ERROR_FILE_WRITE;
        }
        else if (c == '\n')
        {
            if (fputc(c, out) == EOF)
                return STATUS_ERROR_FILE_WRITE;
        }
        else
        {
            if (fprintf(out, "%02X", c) < 0)
                return STATUS_ERROR_FILE_WRITE;
        }
    }
    return STATUS_OK;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        print_error_message(STATUS_ERROR_INVALID_ARG_COUNT);
        fprintf(stderr, "Использование:\n");
        fprintf(stderr, "  %s <-d|-i|-s|-a> <input_file>\n", argv[0]);
        fprintf(stderr, "  %s <-nd|-ni|-ns|-na> <input_file> <output_file>\n", argv[0]);
        return STATUS_ERROR_INVALID_ARG_COUNT;
    }

    char flag = '\0';
    bool is_n_flag = false;

    status_code_t status = parse_flag(argv[1], &flag, &is_n_flag);
    if (status != STATUS_OK)
    {
        print_error_message(status);
        return status;
    }

    if ((is_n_flag && argc != 4) || (!is_n_flag && argc != 3))
    {
        print_error_message(STATUS_ERROR_INVALID_ARG_COUNT);
        return STATUS_ERROR_INVALID_ARG_COUNT;
    }

    const char *input_filename = argv[2];
    char *output_filename = NULL;

    if (is_n_flag)
    {
        size_t len = strlen(argv[3]) + 1;
        output_filename = (char *)malloc(len);
        if (output_filename == NULL)
        {
            print_error_message(STATUS_ERROR_MEMORY_ALLOCATION);
            return STATUS_ERROR_MEMORY_ALLOCATION;
        }
        memcpy(output_filename, argv[3], len);
    }
    else
    {
        status = generate_output_filename(input_filename, &output_filename);
        if (status != STATUS_OK)
        {
            print_error_message(status);
            return status;
        }
    }

    // Проверка на совпадение входного и выходного файлов
    int same = 0;
    status = check_same_files(input_filename, output_filename, &same);
    if (status != STATUS_OK)
    {
        print_error_message(status);
        free(output_filename);
        return status;
    }
    if (same)
    {
        print_error_message(STATUS_ERROR_FILE_OPEN);
        fprintf(stderr, "Ошибка: Входной и выходной файлы не могут совпадать.\n");
        free(output_filename);
        return STATUS_ERROR_FILE_OPEN;
    }

    FILE *in = fopen(input_filename, "r");
    if (in == NULL)
    {
        print_error_message(STATUS_ERROR_FILE_OPEN);
        free(output_filename);
        return STATUS_ERROR_FILE_OPEN;
    }

    FILE *out = fopen(output_filename, "w");
    if (out == NULL)
    {
        print_error_message(STATUS_ERROR_FILE_OPEN);
        fclose(in);
        free(output_filename);
        return STATUS_ERROR_FILE_OPEN;
    }

    switch (flag)
    {
    case 'd':
        status = flag_d(in, out);
        break;
    case 'i':
        status = flag_i(in, out);
        break;
    case 's':
        status = flag_s(in, out);
        break;
    case 'a':
        status = flag_a(in, out);
        break;
    default:
        status = STATUS_ERROR_INVALID_OPTION;
        break;
    }

    if (status != STATUS_OK)
    {
        print_error_message(status);
    }

    fclose(in);
    fclose(out);
    free(output_filename);

    return status;
}