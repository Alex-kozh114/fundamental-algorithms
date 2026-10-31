#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>

typedef enum
{
    OK,
    INVALID_INPUT,
    INVALID_MEMORY,
    ERR_CONVERSION
} status;

static status int_to_roman(int value, char *out, size_t cap)
{
    if (out == NULL || cap == 0)
        return INVALID_INPUT;
    if (value == 0)
    {
        if (cap < 2)
            return INVALID_INPUT;
        out[0] = '0';
        out[1] = '\0';
        return OK;
    }

    int neg = (value < 0);
    unsigned int v = neg ? (unsigned int)(-(long long)value) : (unsigned int)value;

    static const struct
    {
        unsigned int val;
        const char *s;
    } table[] = {
        {1000, "M"}, {900, "CM"}, {500, "D"}, {400, "CD"},
        {100, "C"}, {90, "XC"}, {50, "L"}, {40, "XL"},
        {10, "X"}, {9, "IX"}, {5, "V"}, {4, "IV"}, {1, "I"}
    };

    size_t pos = 0;
    if (neg)
    {
        if (pos + 1 >= cap)
            return INVALID_INPUT;
        out[pos++] = '-';
    }

    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++)
    {
        while (v >= table[i].val)
        {
            size_t l = strlen(table[i].s);
            if (pos + l >= cap)
                return INVALID_INPUT;
            memcpy(out + pos, table[i].s, l);
            pos += l;
            v -= table[i].val;
        }
    }
    out[pos] = '\0';
    return OK;
}

static status uint_to_zeckendorf(unsigned int value, char *out, size_t cap)
{
    if (out == NULL || cap == 0)
        return INVALID_INPUT;

    unsigned int fib[64];
    fib[0] = 1;
    fib[1] = 2;
    size_t n = 2;
    while (n < 64 && fib[n - 1] + fib[n - 2] <= value)
    {
        fib[n] = fib[n - 1] + fib[n - 2];
        n++;
    }

    unsigned char bits[64] = {0};
    unsigned int v = value;
    for (size_t i = n; i > 0; i--)
    {
        if (fib[i - 1] <= v)
        {
            bits[i - 1] = 1;
            v -= fib[i - 1];
        }
    }

    size_t pos = 0;
    for (size_t i = 0; i < n; i++)
    {
        if (pos + 1 >= cap)
            return INVALID_INPUT;
        out[pos++] = bits[i] ? '1' : '0';
    }
    if (pos + 1 >= cap)
        return INVALID_INPUT;
    out[pos++] = '1';
    out[pos] = '\0';
    return OK;
}

static status int_to_base(long long value, int base, int upper, char *out, size_t cap)
{
    if (out == NULL || cap == 0)
        return INVALID_INPUT;
    if (base < 2 || base > 36)
        base = 10;

    const char *digits = upper ? "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               : "0123456789abcdefghijklmnopqrstuvwxyz";

    int neg = (value < 0);
    unsigned long long v = neg ? (unsigned long long)(-(value + 1)) + 1ULL
                               : (unsigned long long)value;

    char tmp[64];
    size_t t = 0;
    if (v == 0)
        tmp[t++] = '0';
    while (v > 0)
    {
        tmp[t++] = digits[v % (unsigned)base];
        v /= (unsigned)base;
    }

    size_t pos = 0;
    if (neg)
    {
        if (pos + 1 >= cap)
            return INVALID_INPUT;
        out[pos++] = '-';
    }
    while (t > 0)
    {
        if (pos + 1 >= cap)
            return INVALID_INPUT;
        out[pos++] = tmp[--t];
    }
    out[pos] = '\0';
    return OK;
}

static status base_string_to_ll(const char *s, int base, long long *out)
{
    if (s == NULL || out == NULL)
        return INVALID_INPUT;
    if (base < 2 || base > 36)
        base = 10;

    size_t i = 0;
    int neg = 0;
    if (s[0] == '-')
    {
        neg = 1;
        i++;
    }
    if (s[i] == '\0')
        return INVALID_INPUT;

    unsigned long long v = 0;
    for (; s[i] != '\0'; i++)
    {
        char c = s[i];
        int d;
        if (c >= '0' && c <= '9')
            d = c - '0';
        else if (c >= 'a' && c <= 'z')
            d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z')
            d = c - 'A' + 10;
        else
            return INVALID_INPUT;
        if (d >= base)
            return INVALID_INPUT;
        v = v * (unsigned)base + (unsigned)d;
    }
    *out = neg ? -(long long)v : (long long)v;
    return OK;
}

static status mem_dump_binary(const unsigned char *bytes, size_t len,
                              char *out, size_t cap)
{
    if (out == NULL || cap == 0 || bytes == NULL)
        return INVALID_INPUT;
    size_t pos = 0;
    for (size_t i = 0; i < len; i++)
    {
        for (int b = 7; b >= 0; b--)
        {
            if (pos + 1 >= cap)
                return INVALID_INPUT;
            out[pos++] = (bytes[i] >> b) & 1 ? '1' : '0';
        }
        if (i + 1 < len)
        {
            if (pos + 1 >= cap)
                return INVALID_INPUT;
            out[pos++] = ' ';
        }
    }
    out[pos] = '\0';
    return OK;
}

typedef struct
{
    FILE *stream;
    char *buf;
    size_t cap;
    size_t pos;
    int failed;
} sink;

static void sink_write(sink *s, const char *data, size_t len)
{
    if (s->failed)
        return;
    if (s->stream)
    {
        if (fwrite(data, 1, len, s->stream) != len)
            s->failed = 1;
    }
    else
    {
        if (s->pos + len + 1 > s->cap)
        {
            s->failed = 1;
            return;
        }
        memcpy(s->buf + s->pos, data, len);
        s->pos += len;
        s->buf[s->pos] = '\0';
    }
}

static void sink_putc(sink *s, char c)
{
    sink_write(s, &c, 1);
}

static void emit_standard(sink *s, char conv, va_list *ap)
{
    char fmt[4] = {'%', conv, 0};
    char tmp[256];

    int needed;
    va_list copy;
    va_copy(copy, *ap);

    switch (conv)
    {
    case 'd':
    case 'i':
    {
        int v = va_arg(copy, int);
        needed = snprintf(tmp, sizeof(tmp), fmt, v);
        break;
    }
    case 'u':
    case 'o':
    case 'x':
    case 'X':
    {
        unsigned int v = va_arg(copy, unsigned int);
        needed = snprintf(tmp, sizeof(tmp), fmt, v);
        break;
    }
    case 'c':
    {
        int v = va_arg(copy, int);
        needed = snprintf(tmp, sizeof(tmp), fmt, v);
        break;
    }
    case 's':
    {
        const char *v = va_arg(copy, const char *);
        needed = snprintf(tmp, sizeof(tmp), fmt, v ? v : "(null)");
        break;
    }
    case 'f':
    case 'e':
    case 'E':
    case 'g':
    case 'G':
    {
        double v = va_arg(copy, double);
        needed = snprintf(tmp, sizeof(tmp), fmt, v);
        break;
    }
    case 'p':
    {
        void *v = va_arg(copy, void *);
        needed = snprintf(tmp, sizeof(tmp), fmt, v);
        break;
    }
    case '%':
    {
        tmp[0] = '%';
        tmp[1] = '\0';
        needed = 1;
        break;
    }
    default:
        va_end(copy);
        s->failed = 1;
        return;
    }
    va_end(copy);

    if (needed < 0)
    {
        s->failed = 1;
        return;
    }
    va_arg(*ap, void *);
    sink_write(s, tmp, (size_t)needed);
}

static int over_vfprintf(FILE *stream, char *buf, size_t cap,
                         const char *fmt, va_list ap)
{
    if (fmt == NULL)
        return -1;
    if (!stream && (!buf || cap == 0))
        return -1;

    sink s = {stream, buf, cap, 0, 0};
    if (buf && cap > 0)
        buf[0] = '\0';

    va_list args;
    va_copy(args, ap);

    size_t i = 0;
    while (fmt[i] != '\0' && !s.failed)
    {
        if (fmt[i] != '%')
        {
            sink_putc(&s, fmt[i++]);
            continue;
        }
        i++;
        if (fmt[i] == '\0')
        {
            s.failed = 1;
            break;
        }

        char c = fmt[i];
        char extra[256];
        status st = OK;

        if (c == 'R' && fmt[i + 1] == 'o')
        {
            int v = va_arg(args, int);
            st = int_to_roman(v, extra, sizeof(extra));
            if (st == OK)
                sink_write(&s, extra, strlen(extra));
            i += 2;
        }
        else if (c == 'Z' && fmt[i + 1] == 'r')
        {
            unsigned int v = va_arg(args, unsigned int);
            st = uint_to_zeckendorf(v, extra, sizeof(extra));
            if (st == OK)
                sink_write(&s, extra, strlen(extra));
            i += 2;
        }
        else if (c == 'C' && (fmt[i + 1] == 'v' || fmt[i + 1] == 'V'))
        {
            int v = va_arg(args, int);
            int base = va_arg(args, int);
            st = int_to_base(v, base, fmt[i + 1] == 'V', extra, sizeof(extra));
            if (st == OK)
                sink_write(&s, extra, strlen(extra));
            i += 2;
        }
        else if (c == 't' && (fmt[i + 1] == 'o' || fmt[i + 1] == 'O'))
        {
            const char *src = va_arg(args, const char *);
            int base = va_arg(args, int);
            long long v = 0;
            st = base_string_to_ll(src, base, &v);
            if (st == OK)
            {
                char ten[64];
                int k = snprintf(ten, sizeof(ten), "%lld", v);
                if (k > 0)
                    sink_write(&s, ten, (size_t)k);
                else
                    st = ERR_CONVERSION;
            }
            i += 2;
        }
        else if (c == 'm' && (fmt[i + 1] == 'i' || fmt[i + 1] == 'u' ||
                              fmt[i + 1] == 'd' || fmt[i + 1] == 'f'))
        {
            char sub = fmt[i + 1];
            unsigned char bytes[16];
            size_t blen = 0;

            if (sub == 'i')
            {
                int v = va_arg(args, int);
                int32_t iv = (int32_t)v;
                memcpy(bytes, &iv, 4);
                blen = 4;
            }
            else if (sub == 'u')
            {
                unsigned int v = va_arg(args, unsigned int);
                uint32_t uv = (uint32_t)v;
                memcpy(bytes, &uv, 4);
                blen = 4;
            }
            else if (sub == 'd')
            {
                double v = va_arg(args, double);
                memcpy(bytes, &v, sizeof(double));
                blen = sizeof(double);
            }
            else
            {
                double tmpd = va_arg(args, double);
                float v = (float)tmpd;
                memcpy(bytes, &v, sizeof(float));
                blen = sizeof(float);
            }

            st = mem_dump_binary(bytes, blen, extra, sizeof(extra));
            if (st == OK)
                sink_write(&s, extra, strlen(extra));
            i += 2;
        }
        else
        {
            emit_standard(&s, c, &args);
            i += 1;
        }

        if (st != OK)
        {
            s.failed = 1;
            break;
        }
    }

    va_end(args);

    if (s.failed)
        return -1;
    if (!stream)
        return (int)s.pos;
    return 0;
}

int overfprintf(FILE *stream, const char *fmt, ...)
{
    if (stream == NULL || fmt == NULL)
        return -1;
    va_list ap;
    va_start(ap, fmt);
    int r = over_vfprintf(stream, NULL, 0, fmt, ap);
    va_end(ap);
    return r;
}

int oversnprintf(char *buf, size_t size, const char *fmt, ...)
{
    if (buf == NULL || size == 0 || fmt == NULL)
        return -1;
    va_list ap;
    va_start(ap, fmt);
    int r = over_vfprintf(NULL, buf, size, fmt, ap);
    va_end(ap);
    return r;
}

int oversprintf(char *buf, const char *fmt, ...)
{
    if (buf == NULL || fmt == NULL)
        return -1;
    va_list ap;
    va_start(ap, fmt);
    int r = over_vfprintf(NULL, buf, 4096, fmt, ap);
    va_end(ap);
    return r;
}

int main(void)
{
    char line[256];

    if (fgets(line, sizeof(line), stdin) == NULL)
    {
        printf("Error: invalid input\n");
        return INVALID_INPUT;
    }

    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n')
        line[len - 1] = '\0';

    if (strstr(line, "%Ro"))
    {
        int v;
        char *p = strstr(line, "%Ro") + 3;
        while (*p == ' ')
            p++;
        if (sscanf(p, "%d", &v) == 1)
            overfprintf(stdout, "%Ro", v);
    }
    else if (strstr(line, "%Zr"))
    {
        unsigned int v;
        char *p = strstr(line, "%Zr") + 3;
        while (*p == ' ')
            p++;
        if (sscanf(p, "%u", &v) == 1)
            overfprintf(stdout, "%Zr", v);
    }
    else if (strstr(line, "%Cv") || strstr(line, "%CV"))
    {
        int v, base;
        char *p = strstr(line, "%C");
        p = strchr(p, ' ') + 1;
        if (sscanf(p, "%d %d", &v, &base) == 2)
        {
            if (strstr(line, "%Cv"))
                overfprintf(stdout, "%Cv", v, base);
            else
                overfprintf(stdout, "%CV", v, base);
        }
    }
    else if (strstr(line, "%to") || strstr(line, "%TO"))
    {
        char s[64];
        int base;
        char *p = strstr(line, "%t");
        p = strchr(p, ' ') + 1;
        if (sscanf(p, "%63s %d", s, &base) == 2)
        {
            if (strstr(line, "%to"))
                overfprintf(stdout, "%to", s, base);
            else
                overfprintf(stdout, "%TO", s, base);
        }
    }
    else if (strstr(line, "%mi"))
    {
        int v;
        char *p = strstr(line, "%mi") + 3;
        while (*p == ' ')
            p++;
        if (sscanf(p, "%d", &v) == 1)
            overfprintf(stdout, "%mi", v);
    }
    else if (strstr(line, "%mu"))
    {
        unsigned int v;
        char *p = strstr(line, "%mu") + 3;
        while (*p == ' ')
            p++;
        if (sscanf(p, "%u", &v) == 1)
            overfprintf(stdout, "%mu", v);
    }
    else if (strstr(line, "%md"))
    {
        double v;
        char *p = strstr(line, "%md") + 3;
        while (*p == ' ')
            p++;
        if (sscanf(p, "%lf", &v) == 1)
            overfprintf(stdout, "%md", v);
    }
    else if (strstr(line, "%mf"))
    {
        float v;
        char *p = strstr(line, "%mf") + 3;
        while (*p == ' ')
            p++;
        if (sscanf(p, "%f", &v) == 1)
            overfprintf(stdout, "%mf", v);
    }
    else if (strstr(line, "%d") || strstr(line, "%s"))
    {
        int i;
        char s[64];
        unsigned int u;
        char *p = strstr(line, "%d");
        if (!p)
            p = strstr(line, "%s");
        p = strchr(p, ' ') + 1;
        if (sscanf(p, "%d %63s %u", &i, s, &u) == 3)
        {
            overfprintf(stdout, "%d %s %x", i, s, u);
        }
    }

    printf("\n");
    return OK;
}