/*
 * fmt.h - small number/text formatting helpers (no printf)
 */
#ifndef FMT_H
#define FMT_H

#include <stdint.h>
#include "rtc.h"

/* Each returns a pointer just past the last character written. */
char *fmt_uint(char *p, uint32_t v, uint8_t width);        /* zero padded */
char *fmt_fixed(char *p, int32_t v, uint8_t decimals);     /* signed */
char *fmt_time(char *p, const rtc_time_t *t);              /* YYYY-MM-DD HH:MM:SS */

/* Parse a decimal number with up to `decimals` fractional digits, scaled by
 * 10^decimals. *ok is cleared on a syntax error. */
int32_t parse_fixed(const char **s, uint8_t decimals, uint8_t *ok);

#endif
