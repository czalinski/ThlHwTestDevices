/*
 * adc.c - current, battery voltage and supply acquisition
 *
 * Current path:
 *
 *   shunt -> INA181A1 (x20, REF = VDD/2 from 10k/10k) -> 1k/100pF -> RA2
 *   RA2 = OPA1IN2- -> internal ladder R1 = 20k, R2 = 300k -> OPA1OUT (RC2)
 *   OPA1 non-inverting input = DAC1 = VDD * 128/256 = VDD/2
 *
 *   Vout = VDD/2 - G * (Vina - VDD/2),   G = 15 * 20k / (20k + 1k) ~ 14.3
 *
 * The 1k series resistor on the board adds to the 20k input resistor, hence
 * the reduced gain; the "ical" calibration takes care of the exact value.
 * Output range 0..VDD covers roughly +/-5.5 A in a 1 mOhm shunt; larger
 * currents saturate (acceptable: this logger is for parasitic draw).
 *
 * The ADC uses VDD as its reference. Both VDD/2 terms (INA181 REF and DAC1)
 * are ratiometric to VDD, so they give a constant count that the "zero"
 * calibration removes. The absolute scale comes from also converting the
 * 2.048 V FVR, which tells the firmware what VDD actually is.
 */
#include "board.h"
#include "adc.h"

static int32_t burst(uint8_t ch)
{
    ADPCH = ch;
    ADCON0bits.GO = 1;
    while (ADCON0bits.GO)
        ;
    /* ADACC is 18-bit; upper bits of ADACCU are copies of the sign bit */
    return ((int32_t)(int8_t)ADACCU << 16) | ((uint16_t)ADACCH << 8) | ADACCL;
}

void adc_init(void)
{
    /* FVR buffer 1 = 2.048 V (measured by the ADC to find VDD) */
    FVRCON = 0x82;                      /* EN, ADFVR = 2x */
    while (!FVRCONbits.RDY)
        ;

    /* DAC1 = VDD/2, internal only. OE must stay 00: OE = 10 would drive RA2,
     * which is the INA181 output. */
    DAC1DATL = 128;
    DAC1CON = 0x80;                     /* EN, OE off, PSS = VDD, NSS = VSS */

    /* OPA1: inverting gain 15 around DAC1 */
    OPA1CON1 = 0x7A;                    /* GSEL = 111 (R1 = 1R, R2 = 15R), RESON, NSS = OPA1IN2- (RA2) */
    OPA1CON2 = 0x14;                    /* NCH = ladder tap, PCH = DAC1_OUT */
    OPA1CON3 = 0x80;                    /* FMS = OPA1OUT pin (ladder top) */
    OPA1HWC = 0x00;                     /* no hardware override */
    OPA1CON0 = 0x80;                    /* EN, charge pump off, basic operation */

    /* ADC: Fosc / (2 * (31 + 1)) = 1 MHz -> TAD = 1 us, VREF+ = VDD */
    ADCLK = 31;
    ADREF = 0x00;
    ADACQL = 0x00; ADACQH = 0x02;       /* 512 Fosc clocks = 8 us acquisition */
    ADPREL = 0; ADPREH = 0;
    ADRPT = ADC_BURST;
    ADCON1 = 0x00;
    ADCON2 = 0x5B;                      /* CRS = 5 (/32), ACLR, MD = burst average */
    ADCON3 = 0x00;
    ADCON0 = 0x84;                      /* ON, Fosc clock, right justified */
}

int32_t adc_current_burst(void)
{
    return burst(ADCH_RC2);
}

int32_t adc_voltage_burst(void)
{
    return burst(ADCH_RA4);
}

int32_t adc_fvr_burst(void)
{
    return burst(ADCH_FVR1);
}
