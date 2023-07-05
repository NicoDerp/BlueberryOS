
#include <unistd.h>
#include <sys/syscall.h>
#include <stdarg.h>

#include <stdio.h>

extern int syscalla0(int, ...);
extern int syscalla1(int, ...);
extern int syscalla2(int, ...);
extern int syscalla3(int, ...);
extern int syscalla4(int, ...);
extern int syscalla5(int, ...);
extern int syscalla6(int, ...);

typedef int (*function_t)(int, ...);
function_t lookup[] = {
    syscalla1, /* SYS_exit          */
    syscalla3, /* SYS_write         */
    syscalla0, /* SYS_yield         */
    syscalla2, /* SYS_open          */
    syscalla3, /* SYS_read          */
    syscalla1, /* SYS_close         */
    syscalla0, /* SYS_fork          */
    syscalla1, /* SYS_waitpid       */
    syscalla2, /* SYS_execvp        */
    syscalla2, /* SYS_getcwd        */
    syscalla1, /* SYS_chdir         */
    syscalla3, /* SYS_getenv        */
    syscalla3, /* SYS_setenv        */
    syscalla4, /* SYS_getdirentries */
    syscalla2, /* SYS_stat          */
    syscalla2, /* SYS_lstat         */
    syscalla6, /* SYS_mmap          */
    syscalla2, /* SYS_munmap        */
    syscalla5, /* SYS_getpwuidr     */
    syscalla5, /* SYS_getgrgidr     */
    syscalla0, /* SYS_getuid        */
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

