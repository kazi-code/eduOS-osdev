
#include <basic.h>
#include <string.h>
// #define DEBUG
#include <debug.h>
#include <arch/x86/cpu.h> // MOVE TO ARCH-DEPENDANT VMM BACKEND
#include "pmm.h"
#include "vmm.h"

//
// More consistent naming for the page tables
//
// P4 = PML4
// P3 = PDPT
// P2 = PD
// P1 = PT
//

#define __vmm_use_recursive

#ifdef __vmm_use_recursive

#define REC_ENTRY (uintptr_t)0400

#define P1_BASE 0xFFFF800000000000
#define P2_BASE (P1_BASE + (REC_ENTRY << 30))
#define P3_BASE (P2_BASE + (REC_ENTRY << 21))
#define P4_BASE (P3_BASE + (REC_ENTRY << 12))

#define P1_STRIDE 0x1000
#define P2_STRIDE 0x200000
#define P3_STRIDE 0x40000000
#define SIZEOF_ENTRY sizeof(usize)

usize *vmm_get_p4_table(usize vma) {
    return (usize *)P4_BASE;
}

usize *vmm_get_p4_entry(usize vma) {
    usize p4_offset = (vma >> 39) & 0777;
    return (usize *)(P4_BASE + p4_offset * SIZEOF_ENTRY);
}

usize *vmm_get_p3_table(usize vma) {
    usize p4_offset = (vma >> 39) & 0777;
    return (usize *)(P3_BASE + p4_offset * P1_STRIDE);
}

usize *vmm_get_p3_entry(usize vma) {
    usize p4_offset = (vma >> 39) & 0777;
    usize p3_offset = (vma >> 30) & 0777;
    return (usize *)(P3_BASE + p4_offset * P1_STRIDE + p3_offset * SIZEOF_ENTRY);
}

usize *vmm_get_p2_table(usize vma) {
    usize p4_offset = (vma >> 39) & 0777;
    usize p3_offset = (vma >> 30) & 0777;
    return (usize *)(P2_BASE + p4_offset * P2_STRIDE + p3_offset * P1_STRIDE);
}

usize *vmm_get_p2_entry(usize vma) {
    usize p4_offset = (vma >> 39) & 0777;
    usize p3_offset = (vma >> 30) & 0777;
    usize p2_offset = (vma >> 21) & 0777;
    return (usize *)(P2_BASE + p4_offset * P2_STRIDE + p3_offset * P1_STRIDE + p2_offset * SIZEOF_ENTRY);
}

usize *vmm_get_p1_table(usize vma) {
    usize p4_offset = (vma >> 39) & 0777;
    usize p3_offset = (vma >> 30) & 0777;
    usize p2_offset = (vma >> 21) & 0777;
    return (usize *)(P1_BASE + p4_offset * P3_STRIDE + p3_offset * P2_STRIDE + p2_offset * P1_STRIDE);
}

usize *vmm_get_p1_entry(usize vma) {
    usize p4_offset = (vma >> 39) & 0777;
    usize p3_offset = (vma >> 30) & 0777;
    usize p2_offset = (vma >> 21) & 0777;
    usize p1_offset = (vma >> 12) & 0777;
    return (usize *)(P1_BASE + p4_offset * P3_STRIDE + p3_offset * P2_STRIDE + p2_offset * P1_STRIDE + p1_offset * SIZEOF_ENTRY);
}

#else /* NOT __vmm_use_recursive */

/*
 * TODO: THIS IS REALLY INEFFICIENT
 *
 * Each level always calls the one above it!!
 */

uintptr_t *vmm_get_p4_table(uintptr_t vma) {
    uintptr_t p4;
    asm volatile ("mov %%cr3, %0" : "=a"(p4));
    return (uintptr_t *)(p4 & PAGE_MASK_4K);
}

uintptr_t *vmm_get_p4_entry(uintptr_t vma) {
    uintptr_t p4_offset = (vma >> 39) & 0777;
    return vmm_get_p4_table(vma) + p4_offset;
}

uintptr_t *vmm_get_p3_table(uintptr_t vma) {
    return (uintptr_t *)(*vmm_get_p4_entry(vma) & PAGE_MASK_4K);
}

uintptr_t *vmm_get_p3_entry(uintptr_t vma) {
    uintptr_t p3_offset = (vma >> 30) & 0777;
    return vmm_get_p3_table(vma) + p3_offset;
}

uintptr_t *vmm_get_p2_table(uintptr_t vma) {
    return (uintptr_t *)(*vmm_get_p3_entry(vma) & PAGE_MASK_4K);
}

uintptr_t *vmm_get_p2_entry(uintptr_t vma) {
    uintptr_t p2_offset = (vma >> 21) & 0777;
    return vmm_get_p2_table(vma) + p2_offset;
}

uintptr_t *vmm_get_p1_table(uintptr_t vma) {
    return (uintptr_t *)(*vmm_get_p2_entry(vma) & PAGE_MASK_4K);
}

uintptr_t *vmm_get_p1_entry(uintptr_t vma) {
    uintptr_t p1_offset = (vma >> 12) & 0777;
    return vmm_get_p1_table(vma) + p1_offset;
}

#endif /* __vmm_use_recursive */


usize vmm_virt_to_phy(usize virtual) {
    DEBUG_PRINTF("resolve %p\n", virtual);

    usize p4 = *vmm_get_p4_entry(virtual);
    DEBUG_PRINTF("p4 entry is %p\n", p4);
    if (!(p4 & PAGE_PRESENT)) return -1;

    usize p3 = *vmm_get_p3_entry(virtual);
    DEBUG_PRINTF("p3 entry is %p\n", p3);
    if (!(p3 & PAGE_PRESENT)) return -1;
    if (p3 & PAGE_ISHUGE) return (p3 & PAGE_MASK_1G) + (virtual & PAGE_OFFSET_1G);

    usize p2 = *vmm_get_p2_entry(virtual);
    DEBUG_PRINTF("p2 entry is %p\n", p2);
    if (!(p2 & PAGE_PRESENT)) return -1;
    if (p2 & PAGE_ISHUGE) return (p2 & PAGE_MASK_2M) + (virtual & PAGE_OFFSET_2M);

    usize p1 = *vmm_get_p1_entry(virtual);
    DEBUG_PRINTF("p1 entry is %p\n", p1);
    if (!(p1 & PAGE_PRESENT)) return -1;
    return (p1 & PAGE_MASK_4K) + (virtual & PAGE_OFFSET_4K);
}

#define FLAGS_DEFAULT (-1)

void make_next_table(usize *table_location, usize flags) {
    if (flags == FLAGS_DEFAULT) {
        flags = PAGE_PRESENT | PAGE_WRITEABLE;
    }
    usize physical = pmm_allocate_page();
    *table_location = physical | flags;
}

bool vmm_map(usize virtual, usize physical) {
    DEBUG_PRINTF("map %p to %p\n", virtual, physical);

    usize *p4_entry = vmm_get_p4_entry(virtual);
    if (!(*p4_entry & PAGE_PRESENT)) {
        DEBUG_PRINTF("Creating new p4 entry and p3 table for %p\n", virtual);

        make_next_table(p4_entry, FLAGS_DEFAULT);
        memset(vmm_get_p3_table(virtual), 0, 0x1000);
    }

    usize *p3_entry = vmm_get_p3_entry(virtual);
    if (*p3_entry & PAGE_ISHUGE) {
        return false; // can't map inside a huge page
    }
    if (!(*p3_entry & PAGE_PRESENT)) {
        DEBUG_PRINTF("Creating new p3 entry and p2 table for %p\n", virtual);

        make_next_table(p3_entry, FLAGS_DEFAULT);
        memset(vmm_get_p2_table(virtual), 0, 0x1000);
    }

    usize *p2_entry = vmm_get_p2_entry(virtual);
    if (*p2_entry & PAGE_ISHUGE) {
        return false; // can't map inside a huge page
    }
    if (!(*p2_entry & PAGE_PRESENT)) {
        DEBUG_PRINTF("Creating new p2 entry and p1 table for %p\n", virtual);

        make_next_table(p2_entry, FLAGS_DEFAULT);
        memset(vmm_get_p1_table(virtual), 0, 0x1000);
    }

    usize *p1_entry = vmm_get_p1_entry(virtual);
    usize default_flags = PAGE_PRESENT | PAGE_WRITEABLE;

    *p1_entry = physical | default_flags;
    return true;
}

// Maps contiguous virtual memory to contiguous physical memory
void vmm_map_range(usize virtual, usize physical, usize len) {
    virtual &= PAGE_MASK_4K;
    physical &= PAGE_MASK_4K;
    len /= 0x1000;
    if (len == 0)  len = 1;

    for (usize i=0; i<len; i++) {
        vmm_map(virtual + i * 0x1000, physical + i * 0x1000);
    }
}

bool vmm_edit_flags(uintptr_t vma, int flags) {
    vma &= PAGE_MASK_4K;
    DEBUG_PRINTF("edit %p\n", vma);

    uintptr_t p4 = *vmm_get_p4_entry(vma);
    DEBUG_PRINTF("p4 entry is %p\n", p4);
    if (!(p4 & PAGE_PRESENT)) return false;

    uintptr_t p3 = *vmm_get_p3_entry(vma);
    DEBUG_PRINTF("p3 entry is %p\n", p3);
    if (!(p3 & PAGE_PRESENT)) return false;
    if (p3 & PAGE_ISHUGE) return (p3 & PAGE_MASK_1G) + (vma & PAGE_OFFSET_1G);

    uintptr_t p2 = *vmm_get_p2_entry(vma);
    DEBUG_PRINTF("p2 entry is %p\n", p2);
    if (!(p2 & PAGE_PRESENT)) return false;
    if (p2 & PAGE_ISHUGE) return (p2 & PAGE_MASK_2M) + (vma & PAGE_OFFSET_2M);

    uintptr_t *p1 = vmm_get_p1_entry(vma);
    DEBUG_PRINTF("p1 entry is %p\n", p1);
    if (!(*p1 & PAGE_PRESENT)) return false;
    uintptr_t tmp_p1 = (*p1 & PAGE_MASK_4K) | flags | PAGE_PRESENT;
    *p1 = tmp_p1;
    invlpg(vma);

    return true;
}

