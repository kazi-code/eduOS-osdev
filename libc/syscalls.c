#include <ng/syscall_consts.h>
#include <dirent.h>
#include <errno.h>
#include <nightingale.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ttyctl.h>
#include <sys/trace.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "autogenerated_syscall_names.c"
#include "syscall.h"

static inline int is_error(intptr_t ret) {
    return (ret < 0 && ret > -0x1000);
}

#define RETURN_OR_SET_ERRNO(ret) \
    if (!is_error(ret)) { \
        return ret; \
    } else { \
        errno = -ret; \
        return -1; \
    }

#include "autogenerated_syscalls_user.c"
