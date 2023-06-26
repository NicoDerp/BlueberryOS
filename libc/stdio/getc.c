
#include <stdio.h>

#include <sys/syscall.h>
#include <unistd.h>


int getc(unsigned int fd) {

    char result;
    syscall(SYS_read, fd, &result, 1);
    return (int) result;
}

