/*
 * rtc.c - DS3231 real-time clock on I2C1 (host mode, 100 kHz)
 *
 * SCL = RC3, SDA = RC4 via PPS. RC3/RC4 are not I2C/SMBus-level pads, so they
 * run with standard Schmitt-trigger levels, which is fine at 3.3 V with the
 * board's 4.7k pull-ups.
 *
 * Host transfers use the module's byte counter: I2C1CNT bytes are sent or
 * received after the address, then the module issues Stop on its own (or, with
 * RSEN set, pauses with MDR = 1 so a Restart can follow).
 */
#include "board.h"
#include "rtc.h"

#define DS3231_ADDR     0xD0            /* 7-bit 0x68, write address */
#define REG_SECONDS     0x00
#define REG_CONTROL     0x0E
#define REG_STATUS      0x0F
#define STATUS_OSF      0x80

#define I2C_TIMEOUT     20000           /* polling loops, a few ms */

static uint8_t i2c_ok;

/* Wait for a condition; on timeout reset the module and mark the transfer bad */
#define WAIT_UNTIL(cond)                                    \
    do {                                                    \
        uint16_t n_ = I2C_TIMEOUT;                          \
        while (!(cond)) {                                   \
            if (--n_ == 0) { i2c_ok = 0; i2c_reset(); return; } \
        }                                                   \
    } while (0)

static void i2c_reset(void)
{
    I2C1CON0bits.EN = 0;
    I2C1STAT1bits.CLRBF = 1;
    I2C1PIR = 0;
    I2C1ERR = 0;
    I2C1CON0bits.EN = 1;
}

/* Write `n` bytes from `buf` (first byte is normally the register address).
 * With `hold` set the bus is kept (no Stop) so a read can follow. */
static void i2c_write(const uint8_t *buf, uint8_t n, uint8_t hold)
{
    I2C1PIR = 0;
    I2C1ERR = 0;
    I2C1STAT1bits.CLRBF = 1;
    I2C1CON0bits.RSEN = hold;
    I2C1ADB1 = DS3231_ADDR;
    I2C1CNTL = n;
    I2C1CNTH = 0;
    I2C1TXB = *buf++;                   /* first data byte */
    I2C1CON0bits.S = 1;
    while (--n) {
        WAIT_UNTIL(I2C1STAT1bits.TXBE || I2C1ERRbits.NACKIF);
        if (I2C1ERRbits.NACKIF)
            break;
        I2C1TXB = *buf++;
    }
    if (hold)
        WAIT_UNTIL(I2C1CON0bits.MDR || I2C1PIRbits.PCIF);
    else
        WAIT_UNTIL(I2C1PIRbits.PCIF);
    if (I2C1ERRbits.NACKIF)
        i2c_ok = 0;
}

/* Read `n` bytes (Restart if the bus is held from a previous write) */
static void i2c_read(uint8_t *buf, uint8_t n)
{
    I2C1CON0bits.RSEN = 0;
    I2C1ADB1 = DS3231_ADDR | 1;
    I2C1CNTL = n;
    I2C1CNTH = 0;
    I2C1CON0bits.S = 1;
    while (n--) {
        WAIT_UNTIL(I2C1STAT1bits.RXBF || I2C1PIRbits.PCIF);
        if (!I2C1STAT1bits.RXBF) {
            i2c_ok = 0;
            return;
        }
        *buf++ = I2C1RXB;
    }
    WAIT_UNTIL(I2C1PIRbits.PCIF);
}

static uint8_t read_regs(uint8_t reg, uint8_t *buf, uint8_t n)
{
    i2c_ok = 1;
    i2c_write(&reg, 1, 1);
    if (i2c_ok)
        i2c_read(buf, n);
    else if (I2C1CON0bits.MDR)
        I2C1CON1bits.P = 1;             /* release the held bus */
    return i2c_ok;
}

static uint8_t write_regs(const uint8_t *buf, uint8_t n)
{
    i2c_ok = 1;
    i2c_write(buf, n, 0);
    return i2c_ok;
}

static uint8_t bcd2bin(uint8_t v) { return (uint8_t)((v >> 4) * 10 + (v & 0x0F)); }
static uint8_t bin2bcd(uint8_t v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

void rtc_init(void)
{
    static const uint8_t ctrl[2] = { REG_CONTROL, 0x1C };   /* osc on, SQW off */

    I2C1CON0 = 0x04;                    /* host mode, 7-bit address (disabled) */
    I2C1CON1 = 0x80;                    /* ACKCNT: NACK the last byte read */
    I2C1CON2 = 0x00;                    /* address buffer, Fscl = clk / 5 */
    I2C1CLK = 0x03;                     /* MFINTOSC 500 kHz -> 100 kHz */
    I2C1CON0bits.EN = 1;

    write_regs(ctrl, 2);
}

uint8_t rtc_read(rtc_time_t *t)
{
    uint8_t b[7];

    if (!read_regs(REG_SECONDS, b, 7))
        return 0;
    t->sec = bcd2bin(b[0] & 0x7F);
    t->min = bcd2bin(b[1] & 0x7F);
    t->hour = bcd2bin(b[2] & 0x3F);     /* 24-hour mode */
    t->day = bcd2bin(b[4] & 0x3F);
    t->mon = bcd2bin(b[5] & 0x1F);
    t->year = bcd2bin(b[6]);
    return 1;
}

uint8_t rtc_write(const rtc_time_t *t)
{
    uint8_t b[8];
    uint8_t st;

    b[0] = REG_SECONDS;
    b[1] = bin2bcd(t->sec);
    b[2] = bin2bcd(t->min);
    b[3] = bin2bcd(t->hour);            /* bit 6 = 0: 24-hour mode */
    b[4] = 1;                           /* day of week (unused) */
    b[5] = bin2bcd(t->day);
    b[6] = bin2bcd(t->mon);             /* century bit 0 */
    b[7] = bin2bcd(t->year);
    if (!write_regs(b, 8))
        return 0;

    /* Clear the oscillator-stop flag */
    if (!read_regs(REG_STATUS, &st, 1))
        return 0;
    b[0] = REG_STATUS;
    b[1] = st & (uint8_t)~STATUS_OSF;
    return write_regs(b, 2);
}

uint8_t rtc_lost_power(void)
{
    uint8_t st;

    return read_regs(REG_STATUS, &st, 1) && (st & STATUS_OSF);
}
