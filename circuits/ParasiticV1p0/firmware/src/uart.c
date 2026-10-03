/*
 * uart.c - UART1 terminal, 115200 8N1, interrupt-driven receive
 */
#include "board.h"
#include "uart.h"

#define RX_SIZE 64                      /* power of two */

static volatile uint8_t rx_buf[RX_SIZE];
static volatile uint8_t rx_head, rx_tail;

void uart_init(void)
{
    /* BRGS = 1: baud = Fosc / (4 * (n + 1)); n = 138 -> 115108 (-0.08%) */
    U1BRGH = 0;
    U1BRGL = 138;
    U1CON0 = 0xB0;                      /* BRGS, TXEN, RXEN, async 8-bit */
    U1CON2 = 0x80;                      /* RUNOVF: keep receiving after overflow */
    U1CON1 = 0x80;                      /* ON */
    PIE4bits.U1RXIE = 1;
}

void uart_isr(void)
{
    uint8_t c, next;

    c = U1RXB;
    next = (rx_head + 1) & (RX_SIZE - 1);
    if (next != rx_tail) {
        rx_buf[rx_head] = c;
        rx_head = next;
    }
}

int16_t uart_getc(void)
{
    uint8_t c;

    if (rx_head == rx_tail)
        return -1;
    c = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) & (RX_SIZE - 1);
    return c;
}

void uart_putc(char c)
{
    while (U1FIFObits.TXBF)
        ;
    U1TXB = (uint8_t)c;
}

void uart_puts(const char *s)
{
    while (*s)
        uart_putc(*s++);
}

void uart_crlf(void)
{
    uart_putc('\r');
    uart_putc('\n');
}
