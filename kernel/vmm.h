
#pragma once
#ifndef NIGHTINGALE_PAGING_H
#define NIGHTINGALE_PAGING_H

#include <basic.h>

/*
 * Mull on this as a potential API for dealing with
 * vmm table entries - it's pretty magic, but it's
 * simple enough and does seem to work.
 */
typedef union PageEntry {
    struct {
        bool present : 1;
        bool writeable : 1;
        bool usermode : 1;
        bool writethrough : 1;
        bool cachedisable : 1;
        bool accessed : 1;
        bool dirty : 1;
        bool ishuge : 1;
        usize ignored : 3;
        bool pat : 1;
        usize address : 51;
    } __attribute__((packed));

    usize value;
} PageEntry;

#define PAGE_PRESENT 0x01
#define PAGE_WRITEABLE 0x02
#define PAGE_USERMODE 0x04
#define PAGE_ACCESSED 0x20
#define PAGE_DIRTY 0x40
#define PAGE_ISHUGE 0x80
#define PAGE_GLOBAL 0x100

#define PAGE_OFFSET_1G 07777777777 // (3 + 3 + 4) * 3 = 30
#define PAGE_OFFSET_2M    07777777 // (3 + 4)     * 3 = 21
#define PAGE_OFFSET_4K       07777 // 4           * 3 = 12

#define PAGE_MASK_1G (~PAGE_OFFSET_1G)
#define PAGE_MASK_2M (~PAGE_OFFSET_2M)
#define PAGE_MASK_4K (~PAGE_OFFSET_4K)

usize vmm_virt_to_phy(usize);
bool vmm_map(usize, usize);
void vmm_map_range(usize, usize, usize);
bool vmm_edit_flags(uintptr_t, int);

#endif
