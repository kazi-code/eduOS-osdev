
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include "unistd.h"

int errno;

uintptr_t syscall1(int syscall_num, uintptr_t arg1) {
    uintptr_t ret;
    asm volatile ("int $0x80" : "=a"(ret), "=c"(errno) : "0"(syscall_num), "D"(arg1));
    return ret;
}

uintptr_t syscall3(int syscall_num, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3) {
    uintptr_t ret;
    asm volatile ("int $0x80" : "=a"(ret), "=c"(errno) :
                  "0"(syscall_num), "D"(arg1), "S"(arg2), "d"(arg3));
    return ret;
}


#define SYS_DEBUGPRINT 1
#define SYS_EXIT 2
#define SYS_READ 4
#define SYS_WRITE 5

void debug_print(const char *message) {
    syscall1(SYS_DEBUGPRINT, (uintptr_t)message);
}

__attribute__((noreturn))
void exit(int status) {
    syscall1(SYS_EXIT, 0);
    __builtin_unreachable();
}

ssize_t read(int fd, void *data, size_t len) {
    ssize_t ret;
    ret = (ssize_t)syscall3(SYS_READ, (uintptr_t)fd, (uintptr_t)data, (uintptr_t)len);
    return ret;
}

ssize_t write(int fd, const void *data, size_t len) {
    ssize_t ret;
    ret = (ssize_t)syscall3(SYS_WRITE, (uintptr_t)fd, (uintptr_t)data, (uintptr_t)len);
    return ret;
}
