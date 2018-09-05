
#pragma once
#ifndef NIGHTINGALE_MULTIBOOT_H
#define NIGHTINGALE_MULTIBOOT_H

#include <basic.h>
#include <multiboot2.h>

void mb_parse(size_t mb_info);
void mb_mmap_print();
size_t mb_mmap_total_usable();
void mb_elf_print();
void *mb_elf_get();
void *mb_acpi_get_rsdp();
void *mb_get_initfs();
void *mb_get_initfs_end();

#endif
