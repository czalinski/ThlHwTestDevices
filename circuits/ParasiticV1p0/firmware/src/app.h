/*
 * app.h - state shared between the measurement loop and the terminal
 */
#ifndef APP_H
#define APP_H

#include <stdint.h>
#include "rtc.h"

#define LOG_INTERVAL_S  10              /* seconds per record */

/* Calibration requests, applied to the next completed record */
#define CAL_NONE        0
#define CAL_ZERO        1               /* current zero (no load current) */
#define CAL_VOLTS       2               /* cal_target = actual mV */
#define CAL_AMPS        3               /* cal_target = actual 0.1 mA */

extern uint8_t cal_req;
extern int32_t cal_target;

extern rtc_time_t now;                  /* last good RTC read */
extern uint8_t rtc_ok;

/* Last completed record */
extern uint8_t rec_valid;
extern int32_t rec_mv;                  /* battery millivolts */
extern int32_t rec_dma;                 /* current, 0.1 mA */
extern int32_t rec_vdd_mv;              /* measured 3.3 V supply */
extern int32_t rec_raw_i;               /* current channel, 1/512 count */
extern uint16_t rec_ticks;

/* Terminal (term.c) */
void term_init(void);
void term_poll(void);
void term_print_record(const char *line, uint8_t len);

#endif
