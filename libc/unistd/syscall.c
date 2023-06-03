
#include <unistd.h>
#include <sys/syscall.h>
#include <stdarg.h>

extern void syscall0(int, ...);
extern void syscall1(int, ...);
extern void syscall2(int, ...);
extern void syscall3(int, ...);

typedef void (*function_t)(int, ...);
function_t lookup[] = {
    syscall1, /* SYS_exit */
    syscall3, /* SYS_write */
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
    va_start(argptr, number);

    lookup[number](number, argptr);

    /*
    switch (lookup[number]) {>
        case 0:
            syscall0(number);
            break;

        case 1:
            int arg1 = va_arg(argptr, int);
            syscall1(number, arg1);
            break

        case 2:
            syscall2(number, arg1, arg2);
            break

        case 3:
            syscall3(number, arg1, arg2, arg3);
            break
    }
    */
    va_end(argptr);

    return 0;
}

