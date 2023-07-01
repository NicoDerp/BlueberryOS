
#include <sys/syscall.h>
#include <unistd.h>


extern int syscall1(int, int);

int close(int fd) {
    return syscall1(SYS_close, fd);
}

