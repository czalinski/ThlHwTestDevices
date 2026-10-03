/*
 * fmt.c - small number/text formatting helpers (no printf)
 */
#include "fmt.h"

char *fmt_uint(char *p, uint32_t v, uint8_t width)
{
    char tmp[10];
    uint8_t n = 0;

    do {
        tmp[n++] = (char)('0' + (uint8_t)(v % 10));
        v /= 10;
    } while (v);
    while (n < width)
        tmp[n++] = '0';
    while (n)
        *p++ = tmp[--n];
    return p;
}

char *fmt_fixed(char *p, int32_t v, uint8_t decimals)
{
    uint32_t u, div = 1;
    uint8_t i;

    if (v < 0) {
        *p++ = '-';
        u = (uint32_t)(-v);
    } else {
        u = (uint32_t)v;
    }
    for (i = 0; i < decimals; i++)
        div *= 10;
    p = fmt_uint(p, u / div, 1);
    if (decimals) {
        *p++ = '.';
        p = fmt_uint(p, u % div, decimals);
    }
    return p;
}

char *fmt_time(char *p, const rtc_time_t *t)
{
    p = fmt_uint(p, 2000u + t->year, 4); *p++ = '-';
    p = fmt_uint(p, t->mon, 2);          *p++ = '-';
    p = fmt_uint(p, t->day, 2);          *p++ = ' ';
    p = fmt_uint(p, t->hour, 2);         *p++ = ':';
    p = fmt_uint(p, t->min, 2);          *p++ = ':';
    p = fmt_uint(p, t->sec, 2);
    return p;
}

int32_t parse_fixed(const char **s, uint8_t decimals, uint8_t *ok)
{
    const char *p = *s;
    int32_t v = 0;
    uint8_t neg = 0, digits = 0, frac = 0, seen_dot = 0;

    while (*p == ' ')
        p++;
    if (*p == '-') {
        neg = 1;
        p++;
    }
    for (;; p++) {
        if (*p >= '0' && *p <= '9') {
            if (seen_dot) {
                if (frac >= decimals)
                    continue;           /* ignore extra precision */
                frac++;
            }
            v = v * 10 + (*p - '0');
            digits++;
        } else if (*p == '.' && !seen_dot) {
            seen_dot = 1;
        } else {
            break;
        }
    }
    if (!digits)
        *ok = 0;
    while (frac < decimals) {
        v *= 10;
        frac++;
    }
    *s = p;
    return neg ? -v : v;
}
