
#include <unistd.h>
#include <sys/syscall.h>
#include <stdarg.h>

#include <stdio.h>

extern int syscalla0(int, ...);
extern int syscalla1(int, ...);
extern int syscalla2(int, ...);
extern int syscalla3(int, ...);

typedef int (*function_t)(int, ...);
function_t lookup[] = {
    syscalla1, /* SYS_exit    */
    syscalla3, /* SYS_write   */
    syscalla0, /* SYS_yield   */
    syscalla2, /* SYS_open    */
    syscalla3, /* SYS_read    */
    syscalla0, /* SYS_fork    */
    syscalla1, /* SYS_waitpid */
    syscalla2, /* SYS_execvp  */
    syscalla2, /* SYS_getcwd  */
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

