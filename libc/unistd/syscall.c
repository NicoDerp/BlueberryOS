
#include <unistd.h>
#include <sys/syscall.h>
#include <stdarg.h>

extern int syscall0(int, ...);
extern int syscall1(int, ...);
extern int syscall2(int, ...);
extern int syscall3(int, ...);

typedef int (*function_t)(int, ...);
function_t lookup[] = {
    syscall1, /* SYS_exit */
    syscall3, /* SYS_write */
    syscall0, /* SYS_yield */
};


//unsigned int lookup[] = {
//    1, /* SYS_exit */
//    3, /* SYS_write */
//};

int syscall(long unsigned int number, ...) {
    if (number >= sizeof(lookup)/sizeof(unsigned int)) {
        return -1;
    }

    va_list argptr;
    int result;

    va_start(argptr, number);
    lookup[number](number, argptr);
    va_end(argptr);

    return result;
}

