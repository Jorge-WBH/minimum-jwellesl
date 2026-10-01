#include "minemu/irq.h"
#include "minemu/platform.h"
#include "minemu/uart.h"

#define UART_RX_BUFFER_SIZE 256

static char rx_buffer[UART_RX_BUFFER_SIZE];
static volatile size_t rx_head; /* next slot the ISR will write to */
static volatile size_t rx_tail; /* next slot the main loop will read from */

void uart_rx_push(char c) {
    size_t next_head = (rx_head + 1) % UART_RX_BUFFER_SIZE;
    if (next_head == rx_tail) {
        /* buffer full: drop the byte rather than overwrite unread data */
        return;
    }
    rx_buffer[rx_head] = c;
    rx_head = next_head;
}

bool uart_rx_pop(char *out) {
    minemu_irq_disable();
    if (rx_head == rx_tail) {
        minemu_irq_enable();
        return false;
    }
    *out = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % UART_RX_BUFFER_SIZE;
    minemu_irq_enable();
    return true;
}

void uart_putc(char c) {
    while (!(MINEMU_UART0->status & MINEMU_UART_STATUS_TX_READY)) {
        // wait until ready
    }
    MINEMU_UART0->tx_data = (uint32_t)c;
}

size_t custom_length(char* x) {
    if (*x == 0) { return 0; }
    else { return 1+custom_length(x+1); }
}

void uart_print(char* string) {
    size_t l = custom_length(string);

    for (size_t i = 0; i < l; i++) {
        while (!(MINEMU_UART0->status & MINEMU_UART_STATUS_TX_READY)) {
            // wait until ready
        }
        MINEMU_UART0->tx_data = (uint32_t)string[i];
    }
}