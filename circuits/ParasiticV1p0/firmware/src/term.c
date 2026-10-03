/*
 * term.c - serial command terminal (115200 8N1, CR or LF ends a line)
 *
 *   help                      list commands
 *   time                      show RTC time
 *   time YYYY-MM-DD HH:MM:SS  set RTC time
 *   status                    last record, logger state, calibration
 *   live on|off               echo records to the terminal
 *   log on|off                start / stop SD logging (stop before pulling the card)
 *   zero                      current zero: run with NO current in the shunt
 *   vcal <volts>              battery voltage calibration, e.g. vcal 12.634
 *   ical <mA>                 current gain calibration, e.g. ical 1000
 *   defaults                  restore default calibration
 */
#include "board.h"
#include "uart.h"
#include "rtc.h"
#include "fmt.h"
#include "settings.h"
#include "logger.h"
#include "app.h"

#define LINE_MAX        32

static char line[LINE_MAX];
static uint8_t len;

static const char help_text[] =
    "time [YYYY-MM-DD HH:MM:SS]\r\n"
    "status\r\n"
    "live on|off\r\n"
    "log on|off\r\n"
    "zero\r\n"
    "vcal <volts>\r\n"
    "ical <mA>\r\n"
    "defaults\r\n";

static const char *const log_names[] = {
    "off", "running", "no card (retrying)", "write error (retrying)"
};

static void prompt(void)
{
    uart_puts("> ");
}

static void put_num(int32_t v, uint8_t decimals)
{
    char buf[12];
    char *p = fmt_fixed(buf, v, decimals);

    *p = 0;
    uart_puts(buf);
}

static void put_kv(const char *k, int32_t v, uint8_t decimals)
{
    uart_puts(k);
    put_num(v, decimals);
    uart_crlf();
}

static uint8_t starts(const char *s, const char *word)
{
    while (*word)
        if (*s++ != *word++)
            return 0;
    return *s == 0 || *s == ' ';
}

static const char *arg(const char *s)
{
    while (*s && *s != ' ')
        s++;
    while (*s == ' ')
        s++;
    return s;
}

static uint8_t on_off(const char *s, uint8_t *ok)
{
    if (starts(s, "on"))
        return 1;
    if (!starts(s, "off"))
        *ok = 0;
    return 0;
}

static void show_time(void)
{
    char buf[20];
    char *p;

    if (!rtc_ok) {
        uart_puts("RTC not responding");
    } else {
        p = fmt_time(buf, &now);
        *p = 0;
        uart_puts(buf);
    }
    uart_crlf();
}

static uint8_t set_time(const char *s)
{
    rtc_time_t t;
    uint8_t ok = 1;
    int32_t y;

    y = parse_fixed(&s, 0, &ok);
    if (*s++ != '-') ok = 0;
    t.mon = (uint8_t)parse_fixed(&s, 0, &ok);
    if (*s++ != '-') ok = 0;
    t.day = (uint8_t)parse_fixed(&s, 0, &ok);
    t.hour = (uint8_t)parse_fixed(&s, 0, &ok);
    if (*s++ != ':') ok = 0;
    t.min = (uint8_t)parse_fixed(&s, 0, &ok);
    if (*s++ != ':') ok = 0;
    t.sec = (uint8_t)parse_fixed(&s, 0, &ok);

    if (!ok || y < 2000 || y > 2099 || t.mon < 1 || t.mon > 12 || t.day < 1 ||
        t.day > 31 || t.hour > 23 || t.min > 59 || t.sec > 59)
        return 0;
    t.year = (uint8_t)(y - 2000);
    return rtc_write(&t);
}

static void show_status(void)
{
    show_time();
    if (rec_valid) {
        put_kv("volts  ", rec_mv, 3);
        put_kv("mA     ", rec_dma, 1);
        put_kv("vdd    ", rec_vdd_mv, 3);
        put_kv("raw_i  ", rec_raw_i, 0);
        put_kv("ticks  ", rec_ticks, 0);
    }
    uart_puts("log    ");
    uart_puts(log_names[log_state]);
    uart_crlf();
    if (log_state == LOG_RUNNING) {
        if (log_file_name()[0]) {
            uart_puts("file   ");
            uart_puts(log_file_name());
            uart_crlf();
            put_kv("bytes  ", (int32_t)log_file_size(), 0);
        }
        put_kv("freeMB ", (int32_t)log_free_mb(), 0);
    }
    put_kv("izero  ", cfg.izero, 0);
    put_kv("igain  ", (int32_t)cfg.igain, 0);
    put_kv("vscale ", cfg.vscale, 0);
}

static void request_cal(uint8_t which, int32_t target)
{
    cal_req = which;
    cal_target = target;
    uart_puts("applies after the next record");
    uart_crlf();
}

static void execute(void)
{
    const char *a = arg(line);
    uint8_t ok = 1;
    int32_t v;

    if (len == 0) {
        /* empty line */
    } else if (starts(line, "help") || starts(line, "?")) {
        uart_puts(help_text);
    } else if (starts(line, "time")) {
        if (*a) {
            if (!set_time(a))
                ok = 0;
            else
                rtc_ok = rtc_read(&now);
        }
        show_time();
    } else if (starts(line, "status")) {
        show_status();
    } else if (starts(line, "live")) {
        v = on_off(a, &ok);
        if (ok) {
            cfg.opts = v ? (cfg.opts | OPT_LIVE) : (cfg.opts & ~OPT_LIVE);
            settings_save();
        }
    } else if (starts(line, "log")) {
        v = on_off(a, &ok);
        if (ok) {
            if (v) {
                cfg.opts |= OPT_LOG;
                log_start();
            } else {
                cfg.opts &= ~OPT_LOG;
                log_stop();
            }
            settings_save();
            uart_puts(log_names[log_state]);
            uart_crlf();
        }
    } else if (starts(line, "zero")) {
        request_cal(CAL_ZERO, 0);
    } else if (starts(line, "vcal")) {
        v = parse_fixed(&a, 3, &ok);
        if (ok && v > 0)
            request_cal(CAL_VOLTS, v);
        else
            ok = 0;
    } else if (starts(line, "ical")) {
        v = parse_fixed(&a, 1, &ok);
        if (ok && v > 0)
            request_cal(CAL_AMPS, v);
        else
            ok = 0;
    } else if (starts(line, "defaults")) {
        settings_defaults();
        settings_save();
    } else {
        ok = 0;
    }
    if (!ok) {
        uart_puts("? (help)");
        uart_crlf();
    }
}

void term_init(void)
{
    uart_crlf();
    uart_puts("ParasiticV1p0 fw " FW_VERSION);
    uart_crlf();
    rtc_ok = rtc_read(&now);
    if (!rtc_ok) {
        uart_puts("RTC not responding");
        uart_crlf();
    } else if (rtc_lost_power()) {
        uart_puts("RTC lost power: set time");
        uart_crlf();
    }
    prompt();
}

void term_poll(void)
{
    int16_t c = uart_getc();

    if (c < 0)
        return;
    if (c == '\r' || c == '\n') {
        if (c == '\n' && len == 0)
            return;                     /* LF after CR */
        uart_crlf();
        line[len] = 0;
        execute();
        len = 0;
        prompt();
    } else if (c == 0x08 || c == 0x7F) {
        if (len) {
            len--;
            uart_puts("\b \b");
        }
    } else if (c >= ' ' && len < LINE_MAX - 1) {
        line[len++] = (char)c;
        uart_putc((char)c);
    }
}

void term_print_record(const char *rec, uint8_t n)
{
    if (!(cfg.opts & OPT_LIVE))
        return;
    uart_putc('\r');                    /* overwrite the prompt */
    while (n--)
        uart_putc(*rec++);
    prompt();
    for (n = 0; n < len; n++)           /* restore any partly typed line */
        uart_putc(line[n]);
}
