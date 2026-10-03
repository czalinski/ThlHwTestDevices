/*
 * logger.h - daily CSV files on the SD card (FatFs)
 */
#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include "rtc.h"

/* Logger states (also shown by "status") */
#define LOG_OFF         0               /* stopped by the user */
#define LOG_RUNNING     1
#define LOG_NO_CARD     2               /* no card / mount failed (retrying) */
#define LOG_ERROR       3               /* write error (retrying) */

extern uint8_t log_state;

void log_start(void);                   /* mount the card */
void log_stop(void);                    /* close the file, unmount */
void log_poll(void);                    /* call once a second: retries after errors */

/* Append one record to the file for date `t` (header added to new files) */
void log_write(const rtc_time_t *t, const char *s, uint8_t len);

const char *log_file_name(void);        /* current file, "" if none */
uint32_t log_file_size(void);
uint32_t log_free_mb(void);             /* free space on the card */

#endif
