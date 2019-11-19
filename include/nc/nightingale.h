
#pragma once
#ifndef _NIGHTINGALE_H_
#define _NIGHTINGALE_H_

#define HEAPDBG_SUMMARY 2
#define HEAPDBG_DETAIL 1

// debug the kernel heap
int heapdbg(int type);

// debug the local heap
void print_pool();
void summarize_pool();

// halt qemu
noreturn int haltvm(int exit_code);

long ng_time();


// TODO: copypasta from kernel/thread
enum procstate {
        PS_COPYFDS = 0x0001,
        PS_SETRUN  = 0x0002,
};

pid_t create(const char *executable);
int procstate(pid_t destination, enum procstate flags);

enum fault_type {
        NULL_DEREF,
};

int fault(enum fault_type type);

#endif // _NIGHTINGALE_H_

