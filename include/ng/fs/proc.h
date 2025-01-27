#pragma once

#include "types.h"

void make_proc_file(
    const char *name, void (*generate)(struct file *, void *arg), void *arg);

struct inode *new_proc_inode(
    int mode, void (*generate)(struct file *, void *arg), void *arg);

void proc_sprintf(struct file *, const char *fmt, ...) __PRINTF(2, 3);
