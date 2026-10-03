/*
 * main.c - ParasiticV1p0 12 V battery voltage / current logger
 *
 * Every 40 ms tick: one 32-conversion burst each of the current, battery
 * voltage and FVR channels is accumulated. When the DS3231 seconds reach a
 * multiple of LOG_INTERVAL_S, the averages become one record:
 *
 *     2026-10-03 10:40:00,12.634,42.3
 *     (end-of-interval timestamp, volts, mA)
 *
 * which is appended to the day's CSV file and echoed to the terminal when
 * "live" is on.
 */
#include "board.h"
#include "uart.h"
#include "rtc.h"
#include "adc.h"
#include "fmt.h"
#include "settings.h"
#include "logger.h"
#include "app.h"

#define MAX_TICKS       300             /* force a record if the RTC stops */
#define TICKS_PER_S     25

uint8_t cal_req;
int32_t cal_target;

rtc_time_t now;
uint8_t rtc_ok;

uint8_t rec_valid;
int32_t rec_mv;
int32_t rec_dma;
int32_t rec_vdd_mv;
int32_t rec_raw_i;
uint16_t rec_ticks;

static volatile uint8_t tick_count;

static int32_t i_sum, v_sum, f_sum;     /* sums of bursts */
static uint16_t n_ticks;
static uint8_t last_sec = 0xFF;
static uint8_t first_interval = 1;      /* skip the partial first interval */
static uint8_t sec_ticks;

void __interrupt() isr(void)
{
    if (PIR3bits.TMR0IF) {
        PIR3bits.TMR0IF = 0;
        tick_count++;
    }
    if (PIR4bits.U1RXIF)
        uart_isr();
}

/* Average of the per-tick burst sums with 4 extra fraction bits: 1/512 count */
static int32_t average(int32_t sum)
{
    return (int32_t)(((int64_t)sum * 16) / n_ticks);
}

static void apply_calibration(int32_t x, int32_t v, int32_t f)
{
    int32_t d = cfg.izero - x;

    switch (cal_req) {
    case CAL_ZERO:
        cfg.izero = x;
        break;
    case CAL_VOLTS:
        if (v > 0)
            cfg.vscale = (uint16_t)(((int64_t)cal_target * f * 1000) / ((int64_t)v * 2048));
        break;
    case CAL_AMPS:
        if (d > 0)
            cfg.igain = (uint32_t)(((int64_t)cal_target * f) / d);
        break;
    default:
        return;
    }
    cal_req = CAL_NONE;
    settings_save();
    uart_puts("\r\ncal saved\r\n");
}

static void finish_record(void)
{
    char line[40];
    char *p;
    int32_t x, v, f;

    if (!n_ticks)
        return;

    x = average(i_sum);
    v = average(v_sum);
    f = average(f_sum);
    if (f <= 0)
        f = 1;

    rec_raw_i = x;
    rec_dma = (int32_t)(((int64_t)(cfg.izero - x) * cfg.igain) / f);
    rec_mv = (int32_t)(((int64_t)v * 2048 * cfg.vscale) / ((int64_t)f * 1000));
    rec_vdd_mv = (int32_t)((2048LL * 4096 * 512) / f);
    rec_ticks = n_ticks;
    rec_valid = 1;
    i_sum = v_sum = f_sum = 0;
    n_ticks = 0;

    apply_calibration(x, v, f);

    p = fmt_time(line, &now);
    *p++ = ',';
    p = fmt_fixed(p, rec_mv, 3);
    *p++ = ',';
    p = fmt_fixed(p, rec_dma, 1);
    *p++ = '\r';
    *p++ = '\n';

    log_write(&now, line, (uint8_t)(p - line));
    term_print_record(line, (uint8_t)(p - line));
}

static void on_tick(void)
{
    i_sum += adc_current_burst();
    v_sum += adc_voltage_burst();
    f_sum += adc_fvr_burst();
    n_ticks++;

    if (++sec_ticks >= TICKS_PER_S) {
        sec_ticks = 0;
        log_poll();
    }

    rtc_ok = rtc_read(&now);
    if (rtc_ok) {
        if (now.sec != last_sec) {
            last_sec = now.sec;
            if (now.sec % LOG_INTERVAL_S == 0) {
                if (first_interval) {
                    first_interval = 0;
                    i_sum = v_sum = f_sum = 0;
                    n_ticks = 0;
                } else {
                    finish_record();
                }
            }
        }
    } else if (n_ticks >= MAX_TICKS) {
        finish_record();                /* keep logging with a stale time */
    }
}

void main(void)
{
    board_init();
    uart_init();
    settings_load();
    rtc_init();
    adc_init();

    INTCON0bits.GIE = 1;

    term_init();
    if (cfg.opts & OPT_LOG)
        log_start();

    for (;;) {
        CLRWDT();
        if (tick_count) {
            INTCON0bits.GIE = 0;
            tick_count--;
            INTCON0bits.GIE = 1;
            on_tick();
        }
        term_poll();
    }
}
