/*
 * board.h - ParasiticV1p0 hardware definitions (PIC18F16Q41-I/SO)
 *
 * The V1.0 PCB was laid out for a PIC16F17144; the PIC18F16Q41 is a drop-in
 * replacement in the same SOIC-20 footprint (identical pin assignment).
 *
 *   pin  port  function
 *    2   RA5   unused (driven low)
 *    3   RA4   ANA4     battery voltage divider (hand-wired rework, see README)
 *    4   RA3   MCLR/VPP (input with weak pull-up, MCLRE = INTMCLR)
 *    5   RC5   unused (driven low)
 *    6   RC4   I2C1 SDA DS3231 RTC (4.7k pull-up)
 *    7   RC3   I2C1 SCL DS3231 RTC (4.7k pull-up)
 *    8   RC6   U1TX     UART terminal (J5.1)
 *    9   RC7   U1RX     UART terminal (J5.2)
 *   10   RB7   SD card chip select (10k pull-up)
 *   11   RB6   SPI1 SCK SD card
 *   12   RB5   SPI1 SDO SD card MOSI
 *   13   RB4   SPI1 SDI SD card MISO
 *   14   RC2   OPA1OUT  current-sense amplifier output -> ADC (no PCB connection)
 *   15   RC1   unused (driven low)
 *   16   RC0   unused (driven low)
 *   17   RA2   OPA1IN2- INA181A1 output via 1k/100pF (current sense)
 *   18   RA1   ICSPCLK (driven low at run time)
 *   19   RA0   ICSPDAT (driven low at run time)
 */
#ifndef BOARD_H
#define BOARD_H

#include <xc.h>
#include <stdint.h>

#define _XTAL_FREQ      64000000UL      /* HFINTOSC 64 MHz */

#define FW_VERSION      "1.0"

/* SD card chip select (active low) */
#define SD_CS_LAT       LATBbits.LATB7

/* ADC positive channel codes (ADPCH) */
#define ADCH_RA4        0x04            /* battery divider */
#define ADCH_RC2        0x12            /* OPA1 output (current) */
#define ADCH_FVR1       0x3E            /* FVR buffer 1 (2.048 V), VDD measurement */

/* PPS input codes: port(A=0,B=1,C=2) << 3 | bit */
#define PPS_IN_RB4      0x0C
#define PPS_IN_RB6      0x0E
#define PPS_IN_RC3      0x13
#define PPS_IN_RC4      0x14
#define PPS_IN_RC7      0x17

/* PPS output codes (RxyPPS) */
#define PPS_OUT_U1TX    0x10
#define PPS_OUT_SCK1    0x1B
#define PPS_OUT_SDO1    0x1C
#define PPS_OUT_SCL1    0x21
#define PPS_OUT_SDA1    0x22

void board_init(void);

#endif
