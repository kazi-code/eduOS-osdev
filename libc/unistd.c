#define UNISTD_NO_GETCWD 1

#include <stdio.h>
#include <sys/ttyctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <nightingale.h>
#include <unistd.h>

int isatty(int fd) { return ttyctl(fd, TTY_ISTTY, 0); }

int execvp(const char *path, char *const argv[])
{
    return execve(path, argv, NULL);
}

off_t lseek(int fd, off_t offset, int whence)
{
    return seek(fd, offset, whence);
}

int sleep(int seconds) { return sleepms(seconds * 1000); }

pid_t wait(int *status) { return waitpid(0, status, 0); }

pid_t clone(int (*fn)(void *), void *new_stack, int flags, void *arg)
{
    return clone0(fn, new_stack, flags, arg);
}

int load_module(int fd)
{
    int loadmod(int fd);
    return loadmod(fd);
}

ssize_t __ng_getcwd(char *, size_t);

char *getcwd(char *buffer, size_t len)
{
    int err = __ng_getcwd(buffer, len);
    if (err < 0)
        return NULL;
    return buffer;
}

int __ng_chdirat(int atfd, const char *path);

int chdir(const char *path) { return chdirat(AT_FDCWD, path); }
int fchdir(int fd) { return chdirat(fd, ""); }
