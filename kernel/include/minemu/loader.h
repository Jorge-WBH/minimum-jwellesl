#ifndef MINEMU_LOADER_H
#define MINEMU_LOADER_H

#include "minemu/boot.h"
#include <stdbool.h>

#define USER_STACK_TOP UINT32_C(0x00800000)

uint32_t minemu_load_usr_program(const struct minemu_boot_info *boot_info);
void minemu_enter_user(uint32_t entry_vaddr, uint32_t stack_top) __attribute__((noreturn));

bool usr_readable(uint32_t address, uint32_t length);
bool usr_writable(uint32_t address, uint32_t length);
bool usr_copy_from(void *destination, uint32_t source, uint32_t length);
bool usr_copy_to(uint32_t destination, const void *source, uint32_t length);

#endif