
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <stdarg.h>


extern int syscall1(int, int);

__attribute__((__noreturn__)) void exit(int status) {
    syscall1(SYS_exit, status);
    __builtin_unreachable();
}

