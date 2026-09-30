#include "minemu/platform.h"

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