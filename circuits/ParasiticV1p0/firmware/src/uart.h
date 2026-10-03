/*
 * uart.h - UART1 terminal, 115200 8N1
 */
#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_init(void);
void uart_isr(void);                    /* call from ISR when U1RXIF set */
int16_t uart_getc(void);                /* -1 if nothing received */
void uart_putc(char c);
void uart_puts(const char *s);
void uart_crlf(void);

#endif
