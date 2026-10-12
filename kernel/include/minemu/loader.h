#ifndef MINEMU_LOADER_H
#define MINEMU_LOADER_H

#include "minemu/boot.h"
#define USER_STACK_TOP UINT32_C(0x00800000)

uint32_t minemu_load_usr_program(const struct minemu_boot_info *boot_info);
void minemu_enter_user(uint32_t entry_vaddr, uint32_t stack_top) __attribute__((noreturn));

#endif