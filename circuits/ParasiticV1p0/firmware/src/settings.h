/*
 * settings.h - calibration and options kept in data EEPROM
 */
#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

#define SET_MAGIC       0x5132          /* bump when the layout changes */

/*
 * Default scale factors (see README "Calibration").
 *
 * Current, in 0.1 mA:  I = (izero - x) * igain / f
 *   x, izero = current-channel average, f = FVR-channel average (same units)
 *   nominal igain = 2048 mV * 10000 / (20 V/V * G) with G = 15*20k/21k
 * Battery, in mV:      V = v * 2048 * vscale / (f * 1000)
 *   vscale = divider ratio x 1000; 100k over 12k -> 112/12 = 9.333
 */
#define IGAIN_NOMINAL   71680UL
#define VSCALE_NOMINAL  9333

#define OPT_LIVE        0x01            /* echo each record on the terminal */
#define OPT_LOG         0x02            /* log to SD card */

typedef struct {
    uint16_t magic;
    int32_t izero;
    uint32_t igain;
    uint16_t vscale;
    uint8_t opts;
    uint8_t check;
} settings_t;

extern settings_t cfg;

void settings_load(void);
void settings_save(void);
void settings_defaults(void);

#endif
