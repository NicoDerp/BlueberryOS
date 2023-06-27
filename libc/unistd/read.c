
#include <sys/syscall.h>
#include <unistd.h>
#include <stddef.h>


extern int syscall3(int, int, int, int);

int read(int fd, void* buf, size_t count) {
    return syscall3(SYS_read, fd, (int) buf, count);
}

