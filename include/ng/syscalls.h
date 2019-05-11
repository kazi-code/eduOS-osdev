
#pragma once
#ifndef NIGHTINGALE_SYSCALLS_H
#define NIGHTINGALE_SYSCALLS_H

#include <ng/basic.h>
#include <ng/syscall.h>
#include <ng/thread.h>
#include <ng/uname.h>
#include <arch/cpu.h>
#include <ng/fs.h>
#include <net/socket.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

sysret sys_exit(int exit_status);
sysret sys_read(int fd, void *buf, size_t len);
sysret sys_write(int fd, void const *buf, size_t len);
sysret sys_fork(interrupt_frame *frame);
sysret sys_top(int show_threads);
sysret sys_getpid(void);
sysret sys_gettid(void);
sysret sys_execve(interrupt_frame *frame, char *file, char **argv,
                              char **envp);
sysret sys_wait4(pid_t);
sysret sys_socket(int, int, int);
sysret sys_strace(bool);
sysret sys_bind(int, struct sockaddr *, size_t);
sysret sys_connect(int, struct sockaddr *, size_t);
sysret sys_send(int fd, const void *buf, size_t len, int flags);
sysret sys_sendto(int fd, const void *buf, size_t len, int flags,
                              const struct sockaddr *, size_t);
sysret sys_recv(int fd, void *buf, size_t len, int flags);
sysret sys_recvfrom(int fd, void *buf, size_t len, int flags,
                                struct sockaddr *, size_t *);
sysret sys_waitpid(pid_t, int *, int);
sysret sys_dup2(int, int);
sysret sys_uname(struct utsname *);
sysret sys_yield(void);
sysret sys_seek(int fs, off_t offset, int whence);
sysret sys_poll(struct pollfd *, nfds_t, int);
sysret sys_mmap(void *, size_t, int, int, int, off_t);
sysret sys_munmap(void *, size_t);
sysret sys_heapdbg(int);
sysret sys_setpgid(void);
sysret sys_exit_group(int);
sysret sys_clone0(interrupt_frame *r, int (*fn)(void *), 
                              void *new_stack, void *arg, int flags);
sysret sys_loadmod(int fd);
sysret sys_haltvm(int exitst);

sysret sys_open(const char *filename, int flags);
sysret sys_read(int fd, void *data, size_t len);
sysret sys_write(int fd, const void *data, size_t len);
sysret sys_dup2(int oldfd, int newfd);
sysret sys_seek(int fs, off_t offset, int whence);


#endif
