/*
 * adc.h - current, battery voltage and supply acquisition
 */
#ifndef ADC_H
#define ADC_H

#include <stdint.h>

#define ADC_BURST       32              /* conversions per burst (ADRPT) */

void adc_init(void);

/* Each returns the sum of ADC_BURST conversions (VREF+ = VDD, 12-bit). */
int32_t adc_current_burst(void);        /* OPA1 output (inverted, gain ~14.3) */
int32_t adc_voltage_burst(void);        /* RA4 battery divider */
int32_t adc_fvr_burst(void);            /* FVR 2.048 V: gives the actual VDD */

#endif
