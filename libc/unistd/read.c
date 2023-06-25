
#include <sys/syscall.h>
#include <unistd.h>

#include <stddef.h>


int read(int fd, void* buf, size_t count) {

    return syscall(SYS_read, fd, buf, count);
}

