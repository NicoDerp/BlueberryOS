
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>


extern int syscall3(int, int, int, int);

int getchar(void) {

    char result;
    syscall3(SYS_read, STDIN_FILENO, (int) &result, 1);
    return (int) result;
}

