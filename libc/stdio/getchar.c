
#include <stdio.h>
#include <sys/syscall.h>


extern int syscall3(int, int, int, int);

int getchar(void) {

    char result;
    syscall3(SYS_read, stdin, (int) &result, 1);
    return (int) result;
}

