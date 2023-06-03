
#include <unistd.h>
#include <sys/syscall.h>
#include <stdarg.h>

extern void syscall0(int, ...);
extern void syscall1(int, ...);
extern void syscall2(int, ...);
extern void syscall3(int, ...);

typedef void (*function_t)(int, ...);
function_t lookup[] = {
    syscall0, /* SYS_exit */
    syscall3, /* SYS_write */
};

int syscall(long unsigned int number, ...) {
    if (number >= sizeof(lookup)/sizeof(unsigned int)) {
        return -1;
    }

    va_list argptr;
    va_start(argptr, number);
    lookup[number](number, argptr);
    va_end(argptr);

    return 0;
}

