enum ng_syscall {
    NG_INVALID,
    NG_EXIT = 2,
    NG_OPEN = 3,
    NG_READ = 4,
    NG_WRITE = 5,
    NG_FORK = 6,
    NG_TOP = 7,
    NG_GETPID = 8,
    NG_GETTID = 9,
    NG_EXECVE = 10,
    NG_STRACE = 15,
    NG_WAITPID = 23,
    NG_DUP2 = 24,
    NG_UNAME = 25,
    NG_YIELD = 26,
    NG_SEEK = 27,
    NG_POLL = 28,
    NG_MMAP = 29,
    NG_MUNMAP = 30,
    NG_SETPGID = 32,
    NG_EXIT_GROUP = 33,
    NG_CLONE0 = 34,
    NG_LOADMOD = 35,
    NG_HALTVM = 36,
    NG_OPENAT = 37,
    NG_EXECVEAT = 38,
    NG_TTYCTL = 39,
    NG_CLOSE = 40,
    NG_PIPE = 41,
    NG_SIGACTION = 42,
    NG_SIGRETURN = 43,
    NG_KILL = 44,
    NG_SLEEPMS = 45,
    NG_GETDIRENTS = 46,
    NG_XTIME = 47,
    NG_CREATE = 48,
    NG_PROCSTATE = 49,
    NG_FAULT = 50,
    NG_TRACE = 51,
    NG_SIGPROCMASK = 52,
    NG_SYSCALL_TEST = 100,
    SYSCALL_MAX,
};
