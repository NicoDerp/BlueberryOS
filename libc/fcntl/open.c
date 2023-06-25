
#include <fcntl.h>

#include <sys/syscall.h>
#include <unistd.h>


int open(const char* filename, int flags) {

    return syscall(SYS_open, filename, flags);
}

