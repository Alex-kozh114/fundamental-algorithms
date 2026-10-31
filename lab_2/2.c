
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#define MAX_NAME_LEN 4096
#define MAX_PATH_LEN 1024
#define MAX_LINE_LEN (MAX_NAME_LEN * 2 + 128)

typedef enum {
    STATUS_OK,
    STATUS_ERR_INVALID_ARGS,
    STATUS_ERR_FILE_OPEN,
    STATUS_ERR_FILE_READ,
    STATUS_ERR_FILE_WRITE,
    STATUS_ERR_MEMORY,
    STATUS_ERR_INVALID_FORMAT,
    STATUS_ERR_VALIDATION
} status_code_t;

typedef struct {
    unsigned int id;
    char name[MAX_NAME_LEN];
    char surname[MAX_NAME_LEN];
    double salary;
} Employee;

static status_code_t check_same_files(const char *path1, const char *path2, int *out_same)
{
    if (path1 == NULL || path2 == NULL || out_same == NULL)
    {
        return STATUS_ERR_INVALID_ARGS;
    }

    char *rp1 = realpath(path1, NULL);
    if (rp1 == NULL)
    {
        return STATUS_ERR_FILE_OPEN;
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

status_code_t validate_string(const char *str) {
    if (str == NULL || str[0] == '\0') {
        return STATUS_ERR_VALIDATION;
    }
    for (size_t i = 0; str[i] != '\0'; ++i) {
        char c = str[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
            return STATUS_ERR_VALIDATION;
        }
    }
    return STATUS_OK;
}

status_code_t validate_salary(double salary) {
    if (salary <= 0.0) {
        return STATUS_ERR_VALIDATION;
    }
    return STATUS_OK;
}

int compare_doubles_eps(double a, double b, double epsilon) {
    double diff = a - b;
    if (diff < -epsilon) {
        return -1;
    } else if (diff > epsilon) {
        return 1;
    }
    return 0;
}

int compare_employees_asc(const void *p1, const void *p2) {
    const Employee *e1 = (const Employee *)p1;
    const Employee *e2 = (const Employee *)p2;

    double epsilon = 1e-7;
    int salary_cmp = compare_doubles_eps(e1->salary, e2->salary, epsilon);
    if (salary_cmp != 0) {
        return salary_cmp;
    }

    int surname_cmp = strcmp(e1->surname, e2->surname);
    if (surname_cmp != 0) {
        return surname_cmp;
    }

    int name_cmp = strcmp(e1->name, e2->name);
    if (name_cmp != 0) {
        return name_cmp;
    }

    if (e1->id < e2->id) return -1;
    if (e1->id > e2->id) return 1;

    return 0;
}

void reverse_employees(Employee *arr, size_t count) {
    size_t left = 0;
    size_t right = (count == 0) ? 0 : count - 1;
    while (left < right) {
        Employee temp = arr[left];
        arr[left] = arr[right];
        arr[right] = temp;
        left++;
        right--;
    }
}

status_code_t read_employees_from_file(const char *filename, Employee **out_array, size_t *out_count) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return STATUS_ERR_FILE_OPEN;
    }

    size_t capacity = 10;
    size_t count = 0;
    Employee *arr = (Employee *)malloc(capacity * sizeof(Employee));
    if (arr == NULL) {
        fclose(file);
        return STATUS_ERR_MEMORY;
    }

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), file) != NULL) {
        unsigned int id;
        char name[MAX_NAME_LEN];
        char surname[MAX_NAME_LEN];
        double salary;

        size_t len = strlen(line);
        if (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[len - 1] = '\0';
        }

        if (line[0] == '\0') {
            continue;
        }

        int parsed = sscanf(line, "%u %s %s %lf", &id, name, surname, &salary);
        if (parsed != 4) {
            free(arr);
            fclose(file);
            return STATUS_ERR_INVALID_FORMAT;
        }

        if (validate_string(name) != STATUS_OK ||
            validate_string(surname) != STATUS_OK ||
            validate_salary(salary) != STATUS_OK) {
            free(arr);
            fclose(file);
            return STATUS_ERR_VALIDATION;
        }

        if (count >= capacity) {
            size_t new_capacity = capacity * 2;
            Employee *new_arr = (Employee *)realloc(arr, new_capacity * sizeof(Employee));
            if (new_arr == NULL) {
                free(arr);
                fclose(file);
                return STATUS_ERR_MEMORY;
            }
            arr = new_arr;
            capacity = new_capacity;
        }

        arr[count].id = id;
        snprintf(arr[count].name, sizeof(arr[count].name), "%s", name);
        snprintf(arr[count].surname, sizeof(arr[count].surname), "%s", surname);
        arr[count].salary = salary;
        count++;
    }

    if (ferror(file)) {
        free(arr);
        fclose(file);
        return STATUS_ERR_FILE_READ;
    }

    fclose(file);
    *out_array = arr;
    *out_count = count;
    return STATUS_OK;
}

status_code_t sort_employees(Employee *arr, size_t count, char flag) {
    if (count > 1) {
        qsort(arr, count, sizeof(Employee), compare_employees_asc);
        if (flag == 'd') {
            reverse_employees(arr, count);
        }
    }
    return STATUS_OK;
}

status_code_t write_employees_to_file(const char *filename, const Employee *arr, size_t count) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        return STATUS_ERR_FILE_OPEN;
    }

    for (size_t i = 0; i < count; ++i) {
        int written = fprintf(file, "%u %s %s %.2f\n",
                              arr[i].id,
                              arr[i].name,
                              arr[i].surname,
                              arr[i].salary);
        if (written < 0) {
            fclose(file);
            return STATUS_ERR_FILE_WRITE;
        }
    }

    if (fclose(file) != 0) {
        return STATUS_ERR_FILE_WRITE;
    }

    return STATUS_OK;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Ошибка: неверное количество аргументов. Использование: %s <входной_файл> <-a|-d> <выходной_файл>\n", argv[0]);
        return 1;
    }

    const char *input_path = argv[1];
    const char *flag_str = argv[2];
    const char *output_path = argv[3];

    if (input_path == NULL || input_path[0] == '\0' || strlen(input_path) >= MAX_PATH_LEN) {
        printf("Ошибка: неверный путь ко входному файлу.\n");
        return 1;
    }
    if (output_path == NULL || output_path[0] == '\0' || strlen(output_path) >= MAX_PATH_LEN) {
        printf("Ошибка: неверный путь к выходному файлу.\n");
        return 1;
    }

    int same_files = 0;
    if (check_same_files(input_path, output_path, &same_files) == STATUS_OK && same_files) {
        printf("Ошибка: входной и выходной файлы не могут совпадать.\n");
        return 1;
    }

    char flag = '\0';
    if ((flag_str[0] == '-' || flag_str[0] == '/') &&
        (flag_str[1] == 'a' || flag_str[1] == 'd' || flag_str[1] == 'A' || flag_str[1] == 'D') &&
        flag_str[2] == '\0') {
        flag = (flag_str[1] == 'A' || flag_str[1] == 'D') ? flag_str[1] + 32 : flag_str[1];
    } else {
        printf("Ошибка: неверный флаг сортировки. Используйте '-a'/'/a' или '-d'/'/d'.\n");
        return 1;
    }

    Employee *employees = NULL;
    size_t count = 0;

    status_code_t status = read_employees_from_file(input_path, &employees, &count);
    if (status != STATUS_OK) {
        switch (status) {
            case STATUS_ERR_FILE_OPEN:
                printf("Ошибка: не удалось открыть входной файл '%s'.\n", input_path);
                break;
            case STATUS_ERR_FILE_READ:
                printf("Ошибка: произошёл сбой при чтении входного файла '%s'.\n", input_path);
                break;
            case STATUS_ERR_MEMORY:
                printf("Ошибка: не удалось выделить память при чтении файла.\n");
                break;
            case STATUS_ERR_INVALID_FORMAT:
                printf("Ошибка: неверный формат файла или структуры данных.\n");
                break;
            case STATUS_ERR_VALIDATION:
                printf("Ошибка: данные во входном файле не прошли проверку.\n");
                break;
            default:
                printf("Ошибка: неизвестная ошибка при чтении файла (код %d).\n", (int)status);
                break;
        }
        return 1;
    }

    status = sort_employees(employees, count, flag);
    if (status != STATUS_OK) {
        printf("Ошибка: не удалось выполнить сортировку.\n");
        free(employees);
        return 1;
    }

    status = write_employees_to_file(output_path, employees, count);
    if (status != STATUS_OK) {
        switch (status) {
            case STATUS_ERR_FILE_OPEN:
                printf("Ошибка: не удалось открыть выходной файл '%s'.\n", output_path);
                break;
            case STATUS_ERR_FILE_WRITE:
                printf("Ошибка: произошёл сбой при записи в выходной файл '%s'.\n", output_path);
                break;
            default:
                printf("Ошибка: неизвестная ошибка при записи файла (код %d).\n", (int)status);
                break;
        }
        free(employees);
        return 1;
    }

    free(employees);
    printf("%zu", sizeof(unsigned int));
    return 0;
}