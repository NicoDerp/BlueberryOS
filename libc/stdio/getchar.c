
#include <stdio.h>

#include <sys/syscall.h>
#include <unistd.h>


int getchar(void) {

    char result;
    syscall(SYS_read, stdin, &result, 1);
    return (int) result;
}

