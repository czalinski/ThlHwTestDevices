/*
 * board.c - configuration bits, clock, pins and the 25 Hz tick timer
 */
#include "board.h"

/* CONFIG1/2 */
#pragma config FEXTOSC = OFF, RSTOSC = HFINTOSC_64MHZ
#pragma config CLKOUTEN = OFF, PR1WAY = OFF, CSWEN = ON, FCMEN = OFF
#pragma config FCMENP = OFF, FCMENS = OFF
/* CONFIG3 */
#pragma config MCLRE = INTMCLR, PWRTS = PWRT_64, MVECEN = OFF, IVT1WAY = OFF
#pragma config LPBOREN = OFF, BOREN = ON
/* CONFIG4 */
#pragma config BORV = VBOR_2P85, ZCD = OFF, PPS1WAY = OFF, STVREN = ON
#pragma config LVP = OFF, XINST = OFF
/* CONFIG5/6: watchdog ~4 s, always on */
#pragma config WDTCPS = WDTCPS_12, WDTE = ON, WDTCWS = WDTCWS_7, WDTCCS = LFINTOSC
/* CONFIG7-10 */
#pragma config BBSIZE = BBSIZE_512, BBEN = OFF, SAFEN = OFF, DEBUG = OFF
#pragma config WRTB = OFF, WRTC = OFF, WRTD = OFF, WRTSAF = OFF, WRTAPP = OFF
#pragma config CP = OFF

/*
 * MCLRE = INTMCLR: on the V1.0 board the MCLR pull-up goes only to the ICSP
 * header VDD pin (J2.2), which is not tied to +3.3V. Using RA3 as a plain input
 * with the internal weak pull-up keeps the part from resetting on a floating
 * MCLR. Programming uses high-voltage (VPP) entry, so LVP is off.
 */

void board_init(void)
{
    OSCFRQ = 0x08;                      /* FRQ = 1000: 64 MHz */

    /* All pins digital outputs low unless listed below */
    LATA = 0; LATB = 0; LATC = 0;
    ANSELA = 0; ANSELB = 0; ANSELC = 0;

    /* PORTA: RA2 (OPA input), RA4 (divider) analog; RA3 input w/ pull-up */
    TRISA = 0x1C;
    ANSELA = 0x14;
    WPUA = 0x08;

    /* PORTB: RB4 SDI input, RB5 SDO, RB6 SCK, RB7 CS outputs */
    SD_CS_LAT = 1;
    TRISB = 0x10;
    WPUB = 0x10;                        /* MISO pull-up when no card */

    /* PORTC: RC2 OPA output (analog), RC3/RC4 I2C (open drain),
     * RC7 RX input, rest outputs */
    TRISC = 0x9C;
    ANSELC = 0x04;
    ODCONC = 0x18;

    /* Peripheral pin select */
    PPSLOCK = 0x55; PPSLOCK = 0xAA; PPSLOCKbits.PPSLOCKED = 0;
    SPI1SCKPPS = PPS_IN_RB6;  RB6PPS = PPS_OUT_SCK1;
    SPI1SDIPPS = PPS_IN_RB4;  RB5PPS = PPS_OUT_SDO1;
    I2C1SCLPPS = PPS_IN_RC3;  RC3PPS = PPS_OUT_SCL1;
    I2C1SDAPPS = PPS_IN_RC4;  RC4PPS = PPS_OUT_SDA1;
    U1RXPPS = PPS_IN_RC7;     RC6PPS = PPS_OUT_U1TX;
    PPSLOCK = 0x55; PPSLOCK = 0xAA; PPSLOCKbits.PPSLOCKED = 1;

    /* Timer0: 8-bit, Fosc/4 / 256 = 62500 Hz, period 250 -> 250 Hz,
     * postscale 1:10 -> 25 Hz (40 ms tick) */
    T0CON1 = 0x48;                      /* CS = Fosc/4, sync, prescale 1:256 */
    TMR0H = 249;
    T0CON0 = 0x89;                      /* enable, 8-bit, postscale 1:10 */
    PIR3bits.TMR0IF = 0;
    PIE3bits.TMR0IE = 1;
}
