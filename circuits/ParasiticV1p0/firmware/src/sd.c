/*
 * sd.c - SD card driver for FatFs on SPI1 (mode 0)
 *
 * Card initialization follows ChaN's generic MMC/SDC driver.
 * SCK1 = RB6, SDO1 = RB5, SDI1 = RB4, CS = RB7 (active low).
 */
#include "board.h"
#include "../lib/fatfs/ff.h"
#include "../lib/fatfs/diskio.h"

/* Card type flags */
#define CT_MMC          0x01
#define CT_SD1          0x02
#define CT_SD2          0x04
#define CT_BLOCK        0x08            /* block addressing (SDHC/SDXC) */

/* Commands */
#define CMD0            (0)             /* GO_IDLE_STATE */
#define CMD1            (1)             /* SEND_OP_COND (MMC) */
#define ACMD41          (0x80 + 41)     /* SEND_OP_COND (SDC) */
#define CMD8            (8)             /* SEND_IF_COND */
#define CMD16           (16)            /* SET_BLOCKLEN */
#define CMD17           (17)            /* READ_SINGLE_BLOCK */
#define CMD24           (24)            /* WRITE_BLOCK */
#define CMD55           (55)            /* APP_CMD */
#define CMD58           (58)            /* READ_OCR */

static uint8_t card_type;
static DSTATUS stat = STA_NOINIT;

static uint8_t spi(uint8_t b)
{
    SPI1TXB = b;
    while (!SPI1STATUSbits.RXBF)
        ;
    return SPI1RXB;
}

#define rcv()   spi(0xFF)

static void spi_setup(uint8_t baud)
{
    SPI1CON0 = 0;                       /* disable while changing settings */
    SPI1CON1 = 0x40;                    /* CKE = 1, CKP = 0 (mode 0), sample middle */
    SPI1CON2 = 0x03;                    /* TXR, RXR: full duplex */
    SPI1CLK = 0;                        /* Fosc */
    SPI1BAUD = baud;                    /* Fsck = Fosc / (2 * (baud + 1)) */
    SPI1TWIDTH = 0;                     /* 8-bit */
    SPI1STATUSbits.CLRBF = 1;
    SPI1CON0 = 0x83;                    /* EN, host, BMODE */
}

/* Wait until the card is not busy (DO high). 1 = ready */
static uint8_t wait_ready(void)
{
    uint16_t n;

    for (n = 50000; n; n--) {           /* ~500 ms */
        if (rcv() == 0xFF)
            return 1;
        __delay_us(10);
        if ((n & 0x3FFF) == 0)
            CLRWDT();
    }
    return 0;
}

static void deselect(void)
{
    SD_CS_LAT = 1;
    rcv();                              /* release DO */
}

static uint8_t select(void)
{
    SD_CS_LAT = 0;
    rcv();
    if (wait_ready())
        return 1;
    deselect();
    return 0;
}

static uint8_t send_raw(uint8_t cmd, uint32_t arg)
{
    uint8_t n, res;

    deselect();
    if (!select())
        return 0xFF;

    spi(0x40 | cmd);
    spi((uint8_t)(arg >> 24));
    spi((uint8_t)(arg >> 16));
    spi((uint8_t)(arg >> 8));
    spi((uint8_t)arg);
    n = 0x01;                           /* dummy CRC + stop */
    if (cmd == CMD0) n = 0x95;
    if (cmd == CMD8) n = 0x87;
    spi(n);

    n = 10;                             /* wait for a valid response */
    do {
        res = rcv();
    } while ((res & 0x80) && --n);
    return res;
}

/* Send a command; ACMD<n> is sent as CMD55 followed by CMD<n> */
static uint8_t send_cmd(uint8_t cmd, uint32_t arg)
{
    uint8_t res;

    if (cmd & 0x80) {
        res = send_raw(CMD55, 0);
        if (res > 1)
            return res;
    }
    return send_raw(cmd & 0x7F, arg);
}

DSTATUS disk_initialize(BYTE pdrv)
{
    uint8_t n, cmd, ty, ocr[4];
    uint16_t tmr;

    if (pdrv)
        return STA_NOINIT;

    CLRWDT();
    spi_setup(79);                      /* 400 kHz for initialization */
    SD_CS_LAT = 1;
    for (n = 10; n; n--)                /* 80 dummy clocks */
        rcv();

    ty = 0;
    if (send_cmd(CMD0, 0) == 1) {
        if (send_cmd(CMD8, 0x1AA) == 1) {           /* SDv2 */
            for (n = 0; n < 4; n++)
                ocr[n] = rcv();
            if (ocr[2] == 0x01 && ocr[3] == 0xAA) {
                for (tmr = 10000; tmr && send_cmd(ACMD41, 1UL << 30); tmr--)
                    __delay_us(100);
                if (tmr && send_cmd(CMD58, 0) == 0) {
                    for (n = 0; n < 4; n++)
                        ocr[n] = rcv();
                    ty = (ocr[0] & 0x40) ? (CT_SD2 | CT_BLOCK) : CT_SD2;
                }
            }
        } else {                                    /* SDv1 or MMC */
            if (send_cmd(ACMD41, 0) <= 1) {
                ty = CT_SD1; cmd = ACMD41;
            } else {
                ty = CT_MMC; cmd = CMD1;
            }
            for (tmr = 10000; tmr && send_cmd(cmd, 0); tmr--)
                __delay_us(100);
            if (!tmr || send_cmd(CMD16, 512) != 0)
                ty = 0;
        }
    }
    card_type = ty;
    deselect();
    CLRWDT();

    if (ty) {
        spi_setup(3);                   /* 8 MHz */
        stat = 0;
    } else {
        stat = STA_NOINIT;
    }
    return stat;
}

DSTATUS disk_status(BYTE pdrv)
{
    return pdrv ? STA_NOINIT : stat;
}

static uint8_t read_block(BYTE *buf, uint32_t sector)
{
    uint8_t rc;
    uint16_t n;

    CLRWDT();                           /* long FAT scans (f_getfree) */
    if (!(card_type & CT_BLOCK))
        sector <<= 9;
    if (send_cmd(CMD17, sector) != 0)
        return 0;
    n = 40000;
    do {
        rc = rcv();
    } while (rc == 0xFF && --n);
    if (rc != 0xFE)
        return 0;
    for (n = 512; n; n--)
        *buf++ = rcv();
    rcv();                              /* CRC */
    rcv();
    return 1;
}

static uint8_t write_block(const BYTE *buf, uint32_t sector)
{
    uint16_t n;

    if (!(card_type & CT_BLOCK))
        sector <<= 9;
    if (send_cmd(CMD24, sector) != 0)
        return 0;
    spi(0xFE);                          /* data token */
    for (n = 512; n; n--)
        spi(*buf++);
    spi(0xFF);                          /* dummy CRC */
    spi(0xFF);
    return (rcv() & 0x1F) == 0x05;      /* data accepted */
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv || !count)
        return RES_PARERR;
    if (stat & STA_NOINIT)
        return RES_NOTRDY;
    while (count) {
        if (!read_block(buff, sector))
            break;
        buff += 512;
        sector++;
        count--;
    }
    deselect();
    return count ? RES_ERROR : RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv || !count)
        return RES_PARERR;
    if (stat & STA_NOINIT)
        return RES_NOTRDY;
    while (count) {
        if (!write_block(buff, sector))
            break;
        buff += 512;
        sector++;
        count--;
    }
    deselect();
    return count ? RES_ERROR : RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    DRESULT res = RES_ERROR;

    (void)buff;
    if (pdrv)
        return RES_PARERR;
    if (stat & STA_NOINIT)
        return RES_NOTRDY;
    if (cmd == CTRL_SYNC) {             /* finish pending write */
        if (select())
            res = RES_OK;
        deselect();
    }
    return res;
}
