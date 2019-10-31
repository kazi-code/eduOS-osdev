
#pragma once
#ifndef NIGHTINGALE_PAGING_H
#define NIGHTINGALE_PAGING_H

#include <ng/basic.h>

#define VMM_VIRTUAL_OFFSET 0x80000000

#define PAGE_PRESENT 0x01
#define PAGE_WRITEABLE 0x02
#define PAGE_USERMODE 0x04
#define PAGE_ACCESSED 0x20
#define PAGE_DIRTY 0x40
#define PAGE_ISHUGE 0x80
#define PAGE_GLOBAL 0x100

#define PAGE_SIZE 0x1000

#define PAGE_OS_RESERVED1 0x200
#define PAGE_OS_RESERVED2 0x400
#define PAGE_OS_RESERVED3 0x800

#define PAGE_COPYONWRITE PAGE_OS_RESERVED1
#define PAGE_STACK_GUARD PAGE_OS_RESERVED2

#define PAGE_UNBACKED 0x100000

#define PAGE_OFFSET_4M 017777777
#define PAGE_OFFSET_4K 07777

#define PAGE_MASK_4M (~PAGE_OFFSET_4M) // check!
#define PAGE_MASK_4K (~PAGE_OFFSET_4K)

#define PAGE_FLAGS_MASK 0x00000FFF
#define PAGE_ADDR_MASK 0xFFFFF000

uintptr_t *vmm_get_pd_table(uintptr_t vma);
uintptr_t *vmm_get_pd_entry(uintptr_t vma);
uintptr_t *vmm_get_pt_table(uintptr_t vma);
uintptr_t *vmm_get_pt_entry(uintptr_t vma);

uintptr_t *vmm_get_pd_table_fork(uintptr_t vma);
uintptr_t *vmm_get_pd_entry_fork(uintptr_t vma);
uintptr_t *vmm_get_pt_table_fork(uintptr_t vma);
uintptr_t *vmm_get_pt_entry_fork(uintptr_t vma);

uintptr_t vmm_virt_to_phy(uintptr_t vma);
uintptr_t vmm_resolve(uintptr_t vma);
bool vmm_map(uintptr_t vma, uintptr_t pma, int flags);
bool vmm_unmap(uintptr_t vma);
void vmm_map_range(uintptr_t vma, uintptr_t pma, size_t len, int flags);
bool vmm_edit_flags(uintptr_t vma, int flags);

void vmm_create(uintptr_t vma, int flags);
void vmm_create_unbacked(uintptr_t vma, int flags);
void vmm_create_unbacked_range(uintptr_t vma, size_t len, int flags);

int vmm_fork();

void vmm_early_init(void);

#endif