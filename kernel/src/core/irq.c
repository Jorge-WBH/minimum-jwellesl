#include "minemu/irq.h"
#include "minemu/platform.h"
#include "minemu/trap.h"
#include "minemu/uart.h"

struct minemu_trap_frame *minemu_irq_dispatch(struct minemu_trap_frame *frame) {
    uint32_t source = (uint32_t)frame->exception_id;

    switch (source) {
    case MINEMU_IRQ_UART0:
        while (MINEMU_UART0->status & MINEMU_UART_STATUS_RX_READY) {
            uart_rx_push((char)MINEMU_UART0->rx_data);
        }
        break;
    default:
        break;
    }

    MINEMU_INTERRUPT->eoi = source;

    return frame;
}
