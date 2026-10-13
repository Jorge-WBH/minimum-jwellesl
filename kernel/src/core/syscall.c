#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "minemu/boot.h"
#include "minemu/mmu.h"
#include "minemu/platform.h"
#include "minemu/syscall.h"
#include "minemu/trace.h"
#include "minemu/trap.h"
#include "minemu/user_abi.h"
#include "minemu/loader.h"
#include "minemu/uart.h"

#define CPSR_MODE_MASK UINT32_C(0x1f)
#define CPSR_MODE_USR UINT32_C(0x10)
#define CPSR_THUMB UINT32_C(0x20)
#define A32_SVC_ZERO UINT32_C(0xef000000)

void minemu_undefined_dispatch(struct minemu_trap_frame *frame) {
    (void)frame;
    minemu_trace_event(UINT32_C(0x48454c03));
    minemu_fail_stop();
}

void minemu_abort_dispatch(struct minemu_trap_frame *frame) {
    (void)frame;
    minemu_trace_event(UINT32_C(0x48454c04));
    minemu_fail_stop();
}

static int32_t uart_read_ioctl(uint32_t address, uint32_t count) {
    uint8_t bytes[MINEMU_IOCTL_MAX_TRANSFER];
    if (count > MINEMU_IOCTL_MAX_TRANSFER ||
        !usr_writable(address, count)) {
        return MINEMU_IOCTL_ERROR;
    }
    uint32_t copied = 0;
    while (copied < count) {
        char x;

        if (!uart_rx_sys_pop(&x)) {
            break;
        }
        bytes[copied++] = (uint8_t)x;
        // bytes[copied++] = (uint8_t)MINEMU_UART0->rx_data;
    }
    return usr_copy_to(address, bytes, copied)
               ? (int32_t)copied
               : MINEMU_IOCTL_ERROR;
}

static int32_t uart_write_ioctl(uint32_t address, uint32_t count) {
    uint8_t bytes[MINEMU_IOCTL_MAX_TRANSFER];
    if (count > MINEMU_IOCTL_MAX_TRANSFER ||
        !usr_copy_from(bytes,address, count)) {
        return MINEMU_IOCTL_ERROR;
    }
    for (uint32_t index = 0; index < count; ++index) {
        while ((MINEMU_UART0->status & MINEMU_UART_STATUS_TX_READY) == 0) {
        }
        MINEMU_UART0->tx_data = bytes[index];
    }
    return (int32_t)count;
}

static int32_t ioctl_dispatch(int32_t fd, uint32_t request,
                              uint32_t argument0, uint32_t argument1) {
    switch (request) {
    case MINEMU_IOCTL_UART_READ:
        return fd == MINEMU_FD_UART_IN
                   ? uart_read_ioctl(argument0, argument1)
                   : MINEMU_IOCTL_ERROR;
    case MINEMU_IOCTL_UART_WRITE:
        return fd == MINEMU_FD_UART_OUT || fd == MINEMU_FD_UART_DIAG
                   ? uart_write_ioctl(argument0, argument1)
                   : MINEMU_IOCTL_ERROR;
    case MINEMU_IOCTL_UART_STATUS: {
        if (fd != MINEMU_FD_UART_IN && fd != MINEMU_FD_UART_OUT &&
            fd != MINEMU_FD_UART_DIAG) {
            return MINEMU_IOCTL_ERROR;
        }
        uint32_t result = 0;
        // RX bytes are drained into the ring by the IRQ handler, so ask the ring
        if (uart_has_byte()) {
            result |= MINEMU_IOCTL_UART_RX_READY;
        }
        if ((MINEMU_UART0->status & MINEMU_UART_STATUS_TX_READY) != 0) {
            result |= MINEMU_IOCTL_UART_TX_READY;
        }
        return (int32_t)result;
    }
    case MINEMU_IOCTL_RNG_NEXT:
    case MINEMU_IOCTL_RNG_STATE: {
        if (fd != MINEMU_FD_RNG ||
            !usr_writable(argument0, sizeof(uint32_t))) {
            return MINEMU_IOCTL_ERROR;
        }
        uint32_t value = request == MINEMU_IOCTL_RNG_NEXT ? MINEMU_RNG->data :
                                                               MINEMU_RNG->state;
        return usr_copy_to(argument0, &value,
                                     sizeof(value))
                   ? 0
                   : MINEMU_IOCTL_ERROR;
    }
    case MINEMU_IOCTL_RNG_SEED:
        if (fd != MINEMU_FD_RNG) {
            return MINEMU_IOCTL_ERROR;
        }
        MINEMU_RNG->seed = argument0;
        return 0;
    case MINEMU_IOCTL_TRACE_EVENT:
        if (fd != MINEMU_FD_TRACE) {
            return MINEMU_IOCTL_ERROR;
        }
        minemu_trace_event(argument0);
        return 0;
    default:
        return MINEMU_IOCTL_ERROR;
    }
}

struct minemu_trap_frame *minemu_svc_dispatch(struct minemu_trap_frame *frame) {
    uint32_t mode = frame->spsr & CPSR_MODE_MASK;
    if (frame->exception_id != MINEMU_EXCEPTION_SVC || mode != CPSR_MODE_USR ||
        (frame->spsr & CPSR_THUMB) != 0) {
        frame->r[0] = (uint32_t)MINEMU_IOCTL_ERROR;
        return frame;
    }

    uint32_t instruction;
    if(!usr_copy_from(&instruction, frame->fault_pc, sizeof(instruction))) {
        frame->r[0] = (uint32_t)MINEMU_IOCTL_ERROR;
        return frame;
    }

    if (instruction != A32_SVC_ZERO) {
        frame->r[0] = (uint32_t)MINEMU_IOCTL_ERROR;
        return frame;
    }

    if (frame->r[7] == MINEMU_SYSCALL_IOCTL) {
        frame->r[0] = (uint32_t)ioctl_dispatch((int32_t)frame->r[0],
                                               frame->r[1], frame->r[2],
                                               frame->r[3]);
        return frame;
    }

    frame->r[0]=MINEMU_IOCTL_ERROR;

    return frame;
}