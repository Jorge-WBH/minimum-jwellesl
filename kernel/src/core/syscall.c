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

// temporary
static int32_t ioctl_dispatch(int32_t fd, uint32_t request, uint32_t arg0, uint32_t arg1) {
    (void)fd;
    (void)request;
    (void)arg0;
    (void)arg1;
    return MINEMU_IOCTL_ERROR;
}

struct minemu_trap_frame *minemu_svc_dispatch(struct minemu_trap_frame *frame) {
    uint32_t mode = frame->spsr & CPSR_MODE_MASK;
    if (frame->exception_id != MINEMU_EXCEPTION_SVC || mode != CPSR_MODE_USR ||
        (frame->spsr & CPSR_THUMB) != 0) {
        frame->r[0] = (uint32_t)MINEMU_IOCTL_ERROR;
        return frame;
    }

    uint32_t instruction = *(const uint32_t *)(uintptr_t)frame->fault_pc;
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