#ifndef MINEMU_UART_H
#define MINEMU_UART_H

#include <stdbool.h>

void uart_putc(char c);
void uart_print(char *string);

/* Called from the UART0 IRQ handler only: pushes one received byte into the
 * RX ring buffer. Drops the byte if the buffer is full. */
void uart_rx_push(char c);

/* Called from normal (non-interrupt) context: pops the oldest received byte
 * into *out. Returns false and leaves *out untouched if the buffer is empty. */
bool uart_rx_pop(char *out);
bool uart_rx_sys_pop(char *out);

/* True when the RX ring buffer holds at least one unread byte. */
bool uart_has_byte(void);

#endif
