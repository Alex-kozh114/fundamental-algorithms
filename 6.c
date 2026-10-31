#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

typedef enum
{
    OK = 0,
    INVALID_INPUT,
    ERR_CONVERSION
} status;

static int roman_char_value(char c)
{
    switch (c)
    {
    case 'I':
        return 1;
    case 'V':
        return 5;
    case 'X':
        return 10;
    case 'L':
        return 50;
    case 'C':
        return 100;
    case 'D':
        return 500;
    case 'M':
        return 1000;
    default:
        return -1;
    }
}

static int read_roman(FILE *stream, int *out)
{
    int total = 0, prev = 0, got = 0;
    int c;
    while ((c = fgetc(stream)) != EOF && roman_char_value((char)c) >= 0)
    {
        int v = roman_char_value((char)c);
        total += v;
        if (prev < v)
            total -= 2 * prev;
        prev = v;
        got = 1;
    }
    if (c != EOF)
        ungetc(c, stream);
    if (!got)
        return -1;
    *out = total;
    return 0;
}

static int read_zeckendorf(FILE *stream, unsigned int *out)
{
    unsigned int fib[64];
    fib[0] = 1;
    fib[1] = 2;
    size_t n = 2;
    while (n < 64)
    {
        fib[n] = fib[n - 1] + fib[n - 2];
        n++;
    }

    char bits[128];
    size_t len = 0;
    int c;
    int prev = 0;
    while ((c = fgetc(stream)) != EOF && (c == '0' || c == '1'))
    {
        if (len + 1 >= sizeof(bits))
            return -1;
        bits[len++] = (char)c;
        if (prev == 1 && c == '1')
        {
            break;
        }
        prev = c - '0';
    }
    if (c != EOF && c != '1')
        ungetc(c, stream);
    if (len == 0 || bits[len - 1] != '1')
        return -1;

    unsigned int value = 0;
    for (size_t i = 0; i + 1 < len; i++)
    {
        if (i >= n)
            return -1;
        if (bits[i] == '1')
            value += fib[i];
    }
    *out = value;
    return 0;
}

static int read_base_number(FILE *stream, int base, int upper, int *out)
{
    if (base < 2 || base > 36)
        base = 10;

    int c;
    while ((c = fgetc(stream)) != EOF && isspace(c))
    {
    }
    if (c == EOF)
        return -1;

    int neg = 0;
    if (c == '-')
    {
        neg = 1;
        c = fgetc(stream);
    }

    long long value = 0;
    int got = 0;
    while (c != EOF)
    {
        int d;
        if (c >= '0' && c <= '9')
            d = c - '0';
        else if (c >= 'a' && c <= 'z')
            d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z')
            d = c - 'A' + 10;
        else
            break;
        if (d >= base)
            break;
        if (upper && c >= 'a' && c <= 'z')
            break;
        if (!upper && c >= 'A' && c <= 'Z')
            break;
        value = value * base + d;
        got = 1;
        c = fgetc(stream);
    }
    if (c != EOF)
        ungetc(c, stream);
    if (!got)
        return -1;

    *out = (int)(neg ? -value : value);
    return 0;
}

static int over_vfscanf(FILE *stream, const char *fmt, va_list ap)
{
    if (stream == NULL || fmt == NULL)
        return -1;

    va_list args;
    va_copy(args, ap);

    int assigned = 0;
    size_t i = 0;
    while (fmt[i] != '\0')
    {
        if (isspace((unsigned char)fmt[i]))
        {
            int c;
            while ((c = fgetc(stream)) != EOF && isspace(c))
            {
            }
            if (c != EOF)
                ungetc(c, stream);
            i++;
            continue;
        }

        if (fmt[i] != '%')
        {
            int c = fgetc(stream);
            if (c != (unsigned char)fmt[i])
            {
                if (c != EOF)
                    ungetc(c, stream);
                break;
            }
            i++;
            continue;
        }

        i++;
        if (fmt[i] == '\0')
            break;

        char conv = fmt[i];
        int result = -1;

        if (conv == 'R' && fmt[i + 1] == 'o')
        {
            int *p = va_arg(args, int *);
            int v;
            result = read_roman(stream, &v);
            if (result == 0)
            {
                *p = v;
                assigned++;
            }
            i += 2;
        }
        else if (conv == 'Z' && fmt[i + 1] == 'r')
        {
            unsigned int *p = va_arg(args, unsigned int *);
            unsigned int v;
            result = read_zeckendorf(stream, &v);
            if (result == 0)
            {
                *p = v;
                assigned++;
            }
            i += 2;
        }
        else if (conv == 'C' && (fmt[i + 1] == 'v' || fmt[i + 1] == 'V'))
        {
            int upper = (fmt[i + 1] == 'V');
            int *p = va_arg(args, int *);
            int base = va_arg(args, int);
            int v;
            result = read_base_number(stream, base, upper, &v);
            if (result == 0)
            {
                *p = v;
                assigned++;
            }
            i += 2;
        }
        else
        {
            break;
        }

        if (result != 0)
            break;
    }

    va_end(args);
    return assigned;
}

int overfscanf(FILE *stream, const char *fmt, ...)
{
    if (stream == NULL || fmt == NULL)
        return -1;
    va_list ap;
    va_start(ap, fmt);
    int r = over_vfscanf(stream, fmt, ap);
    va_end(ap);
    return r;
}

int oversscanf(const char *s, const char *fmt, ...)
{
    if (s == NULL || fmt == NULL)
        return -1;

    FILE *mem = fmemopen((void *)s, strlen(s), "r");
    if (!mem)
        return -1;

    va_list ap;
    va_start(ap, fmt);
    int r = over_vfscanf(mem, fmt, ap);
    va_end(ap);

    fclose(mem);
    return r;
}

int main(void)
{
    char line[256];

    if (fgets(line, sizeof(line), stdin) == NULL)
    {
        return INVALID_INPUT;
    }

    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n')
        line[len - 1] = '\0';

    char *pos;

    if ((pos = strstr(line, "%Ro")) != NULL)
    {
        char input[128];
        char *p = pos + 3;
        while (*p == ' ')
            p++;

        if (sscanf(p, "%127s", input) == 1)
        {
            int v = 0;
            int n = oversscanf(input, "%Ro", &v);
            if (n == 1)
                printf("%d\n", v);
            else
                printf("Error\n");
        }
        else
        {
            printf("Error\n");
        }
    }
    else if ((pos = strstr(line, "%Zr")) != NULL)
    {
        char input[128];
        char *p = pos + 3;
        while (*p == ' ')
            p++;

        if (sscanf(p, "%127s", input) == 1)
        {
            unsigned int v = 0;
            int n = oversscanf(input, "%Zr", &v);
            if (n == 1)
                printf("%u\n", v);
            else
                printf("Error\n");
        }
        else
        {
            printf("Error\n");
        }
    }
    else if ((pos = strstr(line, "%Cv")) != NULL ||
             (pos = strstr(line, "%CV")) != NULL)
    {
        int upper = (pos[2] == 'V');
        int base;
        char input[128];
        char *p = pos + 3;
        while (*p == ' ')
            p++;

        if (sscanf(p, "%d %127s", &base, input) == 2)
        {
            int v = 0;
            int n;
            if (upper)
                n = oversscanf(input, "%CV", &v, base);
            else
                n = oversscanf(input, "%Cv", &v, base);

            if (n == 1)
                printf("%d\n", v);
            else
                printf("Error\n");
        }
        else
        {
            printf("Error\n");
        }
    }

    return OK;
}