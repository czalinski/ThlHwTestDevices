/*
 * rtc.h - DS3231 real-time clock on MSSP2 (I2C)
 */
#ifndef RTC_H
#define RTC_H

#include <stdint.h>

typedef struct {
    uint8_t year;                       /* 0..99 -> 2000..2099 */
    uint8_t mon;                        /* 1..12 */
    uint8_t day;                        /* 1..31 */
    uint8_t hour;                       /* 0..23 */
    uint8_t min;                        /* 0..59 */
    uint8_t sec;                        /* 0..59 */
} rtc_time_t;

void rtc_init(void);
uint8_t rtc_read(rtc_time_t *t);        /* 1 = ok */
uint8_t rtc_write(const rtc_time_t *t); /* 1 = ok; also clears OSF */
uint8_t rtc_lost_power(void);           /* 1 = oscillator-stop flag set */

#endif
