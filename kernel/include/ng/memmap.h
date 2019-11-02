
#ifndef NG_ARCH_MEMMAP_H
#define NG_ARCH_MEMMAP_H

#include <ng/basic.h>

#if 0
// TODO
#if X86_64
#include <ng/x86/64/memmap.h>
#elif I686
#include <ng/x86/32/memmap.h>
#endif
#endif

#if X86_64

#define KERNEL_RESERVABLE_SPACE 0xFFFFFFF000000000
#define USER_STACK                  0x7FFFFF000000 // 0000 - 0FFF is guard
#define USER_ARGV                   0x7FFFFF001000
#define USER_ENVP                   0x7FFFFF002000
#define USER_MMAP_BASE              0x700000000000

#elif I686

#define KERNEL_RESERVABLE_SPACE         0xC0000000
#define USER_STACK                      0x7FFF0000 // 0000 - 0FFF is guard
#define USER_ARGV                       0x7FFF1000
#define USER_ENVP                       0x7FFF2000
#define USER_MMAP_BASE                  0x50000000

#endif

#endif // NG_ARCH_MMAP_H

