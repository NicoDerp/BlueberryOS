
#include <stdio.h>
#include <sys/syscall.h>


extern int syscall3(int, int, int, int);

int getc(unsigned int fd) {

    char result;
    syscall3(SYS_read, fd, (int) &result, 1);
    return (int) result;
}

