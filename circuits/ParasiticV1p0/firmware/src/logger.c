/*
 * logger.c - daily CSV files on the SD card (FatFs)
 *
 * Records go to YYYYMMDD.CSV in the card's root directory, named from the RTC
 * date of the record. A new file starts with a "time,volts,mA" header. The
 * file is synced after every record, so pulling the card or losing power
 * loses at most the record being written. After any card error the logger
 * unmounts and retries every RETRY_S seconds, so a card can be swapped
 * without touching the terminal (though "log off" first is cleaner).
 */
#include <string.h>
#include "board.h"
#include "logger.h"
#include "fmt.h"
#include "app.h"
#include "../lib/fatfs/ff.h"

#define RETRY_S         30

uint8_t log_state;

static FATFS fs;
static FIL fil;
static uint8_t file_open;
static char file_name[13];              /* "YYYYMMDD.CSV" */
static uint8_t retry_timer;

/* FatFs timestamp callback: local time from the RTC */
DWORD get_fattime(void)
{
    return ((DWORD)(now.year + 20) << 25) | ((DWORD)now.mon << 21) |
           ((DWORD)now.day << 16) | ((DWORD)now.hour << 11) |
           ((DWORD)now.min << 5) | (now.sec >> 1);
}

static void fail(uint8_t state)
{
    if (file_open)
        f_close(&fil);
    file_open = 0;
    file_name[0] = 0;
    f_unmount("");
    log_state = state;
    retry_timer = RETRY_S;
}

void log_start(void)
{
    if (log_state == LOG_RUNNING)
        return;
    file_open = 0;
    file_name[0] = 0;
    if (f_mount(&fs, "", 1) != FR_OK) {
        fail(LOG_NO_CARD);
        return;
    }
    log_state = LOG_RUNNING;
}

void log_stop(void)
{
    if (file_open)
        f_close(&fil);
    file_open = 0;
    file_name[0] = 0;
    f_unmount("");
    log_state = LOG_OFF;
}

void log_poll(void)
{
    if ((log_state == LOG_NO_CARD || log_state == LOG_ERROR) && retry_timer) {
        if (--retry_timer == 0)
            log_start();
    }
}

static uint8_t open_for(const rtc_time_t *t)
{
    char name[13];
    char *p;
    UINT bw;

    p = fmt_uint(name, 2000u + t->year, 4);
    p = fmt_uint(p, t->mon, 2);
    p = fmt_uint(p, t->day, 2);
    memcpy(p, ".CSV", 5);

    if (file_open && strcmp(name, file_name) == 0)
        return 1;
    if (file_open) {
        f_close(&fil);
        file_open = 0;
    }
    if (f_open(&fil, name, FA_WRITE | FA_OPEN_APPEND) != FR_OK)
        return 0;
    file_open = 1;
    strcpy(file_name, name);
    if (f_size(&fil) == 0 &&
        (f_write(&fil, "time,volts,mA\r\n", 15, &bw) != FR_OK || bw != 15))
        return 0;
    return 1;
}

void log_write(const rtc_time_t *t, const char *s, uint8_t len)
{
    UINT bw;

    if (log_state != LOG_RUNNING)
        return;
    if (!open_for(t) || f_write(&fil, s, len, &bw) != FR_OK || bw != len ||
        f_sync(&fil) != FR_OK)
        fail(LOG_ERROR);
}

const char *log_file_name(void)
{
    return file_name;
}

uint32_t log_file_size(void)
{
    return file_open ? f_size(&fil) : 0;
}

uint32_t log_free_mb(void)
{
    FATFS *f;
    DWORD clusters;

    if (log_state != LOG_RUNNING || f_getfree("", &clusters, &f) != FR_OK)
        return 0;
    return (clusters * f->csize) >> 11;   /* 2048 sectors per MiB */
}
