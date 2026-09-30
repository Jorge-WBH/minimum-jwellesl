#include "minemu/boot.h"
#include "minemu/trap.h"
#include "minemu/trace.h"
#include "minemu/platform.h" // needed for the MINEMU_UART0 stuff
#include "minemu/uart.h"

// no standard libraries :(
size_t length(char* x) {
    if (*x == 0) { return 0; }
    else { return 1+length(x+1); }
}

void minemu_kernel_main(const struct minemu_boot_info *boot_info) {
    if ((uintptr_t)boot_info != MINEMU_BOOT_INFO_VADDR ||
        boot_info->magic != MINEMU_BOOT_INFO_MAGIC ||
        boot_info->version != MINEMU_ABI_VERSION ||
        boot_info->size != sizeof(*boot_info) ||
        boot_info->system_rom_base != UINT32_C(0x08000000) ||
        boot_info->direct_map_vaddr != UINT32_C(0xc0000000) ||
        boot_info->direct_map_paddr != UINT32_C(0x40000000) ||
        boot_info->direct_map_size != UINT32_C(0x04000000)) {
        minemu_trace_event(UINT32_C(0xb007bad0));
        minemu_fail_stop();
    }
    // say hello to the world
    char* string = "hello world\n";

    for (size_t i = 0; i < length(string); i++) {
        // first check if TX is ready
        while (!(MINEMU_UART0->status & MINEMU_UART_STATUS_TX_READY)) {
            // wait until ready
        }
        MINEMU_UART0->tx_data = (uint32_t)string[i];
    }    

    // uart_print(string);
    // using the uart driver

    minemu_trace_event(1);
    minemu_fail_stop();
}
