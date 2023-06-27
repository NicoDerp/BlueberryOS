
#include <sys/syscall.h>
#include <unistd.h>
#include <stddef.h>


extern int syscall3(int, int, int, int);

int write(int fd, const void* buf, size_t count) {
    return syscall3(SYS_write, fd, (int) buf, count);
}


