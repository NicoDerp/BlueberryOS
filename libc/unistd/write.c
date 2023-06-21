
#include <sys/syscall.h>
#include <unistd.h>
#include <stddef.h>


int write(int fd, const void* buf, size_t count) {
    return syscall(SYS_write, fd, buf, count);
}


