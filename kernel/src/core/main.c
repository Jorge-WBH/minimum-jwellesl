#include "minemu/boot.h"
#include "minemu/irq.h"
#include "minemu/trap.h"
#include "minemu/trace.h"
#include "minemu/platform.h" // needed for the MINEMU_UART0 stuff
#include "minemu/uart.h"
#include "minemu/msh.h"
#include "minemu/loader.h" // the loader we made

void minemu_kernel_main(const struct minemu_boot_info *boot_info)
{
    if ((uintptr_t)boot_info != MINEMU_BOOT_INFO_VADDR ||
        boot_info->magic != MINEMU_BOOT_INFO_MAGIC ||
        boot_info->version != MINEMU_ABI_VERSION ||
        boot_info->size != sizeof(*boot_info) ||
        boot_info->system_rom_base != UINT32_C(0x08000000) ||
        boot_info->direct_map_vaddr != UINT32_C(0xc0000000) ||
        boot_info->direct_map_paddr != UINT32_C(0x40000000) ||
        boot_info->direct_map_size != UINT32_C(0x04000000))
    {
        minemu_trace_event(UINT32_C(0xb007bad0));
        minemu_fail_stop();
    }

    uint32_t usr = minemu_load_usr_program(boot_info);


    // Unmask UART0 at the interrupt controller, enable RX interrupts on the
    // UART peripheral itself, then unmask IRQs on the CPU. All three are
    // required: this doesn't fire without all of them set.
    MINEMU_INTERRUPT->enable |= UINT32_C(1) << MINEMU_IRQ_UART0;
    MINEMU_UART0->control |= MINEMU_UART_CONTROL_RX_IRQ_ENABLE;
    minemu_irq_enable();

    minemu_trace_event(1);
    minemu_enter_user(usr, USER_STACK_TOP);
}
