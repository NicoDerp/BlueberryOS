
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>


__attribute__((__noreturn__)) void exit(int status) {
    syscall(SYS_exit, status);
    __builtin_unreachable();
}

