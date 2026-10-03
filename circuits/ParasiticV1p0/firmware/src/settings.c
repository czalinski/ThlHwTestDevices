/*
 * settings.c - calibration and options kept in data EEPROM
 *
 * Data EEPROM (512 bytes) is at 0x380000; byte operations use NVMCMD.
 */
#include "board.h"
#include "settings.h"

settings_t cfg;

static void ee_addr(uint8_t addr)
{
    NVMADRU = 0x38;
    NVMADRH = 0x00;
    NVMADRL = addr;
}

static uint8_t ee_read(uint8_t addr)
{
    ee_addr(addr);
    NVMCON1bits.NVMCMD = 0;             /* read byte */
    NVMCON0bits.GO = 1;
    return NVMDATL;
}

static void ee_write(uint8_t addr, uint8_t v)
{
    uint8_t gie;

    if (ee_read(addr) == v)
        return;
    ee_addr(addr);
    NVMDATL = v;
    NVMCON1bits.NVMCMD = 3;             /* write byte */
    gie = INTCON0bits.GIE;
    INTCON0bits.GIE = 0;
    NVMLOCK = 0x55;
    NVMLOCK = 0xAA;
    NVMCON0bits.GO = 1;
    while (NVMCON0bits.GO)
        ;
    INTCON0bits.GIE = gie;
    NVMCON1bits.NVMCMD = 0;
}

static uint8_t checksum(void)
{
    const uint8_t *p = (const uint8_t *)&cfg;
    uint8_t i, s = 0xA5;

    for (i = 0; i < sizeof(cfg) - 1; i++)
        s += p[i];
    return s;
}

void settings_defaults(void)
{
    cfg.magic = SET_MAGIC;
    cfg.izero = 0;
    cfg.igain = IGAIN_NOMINAL;
    cfg.vscale = VSCALE_NOMINAL;
    cfg.opts = OPT_LIVE | OPT_LOG;
}

void settings_load(void)
{
    uint8_t *p = (uint8_t *)&cfg;
    uint8_t i;

    for (i = 0; i < sizeof(cfg); i++)
        p[i] = ee_read(i);
    if (cfg.magic != SET_MAGIC || cfg.check != checksum())
        settings_defaults();
}

void settings_save(void)
{
    const uint8_t *p = (const uint8_t *)&cfg;
    uint8_t i;

    cfg.check = checksum();
    for (i = 0; i < sizeof(cfg); i++)
        ee_write(i, p[i]);
}
