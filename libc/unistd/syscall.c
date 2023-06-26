
#include <unistd.h>
#include <sys/syscall.h>
#include <stdarg.h>

#include <stdio.h>

extern int syscall0(int, ...);
extern int syscall1(int, ...);
extern int syscall2(int, ...);
extern int syscall3(int, ...);

typedef int (*function_t)(int, ...);
function_t lookup[] = {
    syscall1, /* SYS_exit    */
    syscall3, /* SYS_write   */
    syscall0, /* SYS_yield   */
    syscall2, /* SYS_open    */
    syscall3, /* SYS_read    */
    syscall0, /* SYS_fork    */
    syscall1, /* SYS_waitpid */
};


int syscall(long unsigned int number, ...) {
    if (number >= sizeof(lookup)/sizeof(unsigned int)) {
        printf("[ERROR] Syscall overflow!\n");
        for (;;) {}
        return -1;
    }

    int result;
    va_list argptr;

    va_start(argptr, number);
    result = lookup[number](number, argptr);
    va_end(argptr);

    return result;
}

