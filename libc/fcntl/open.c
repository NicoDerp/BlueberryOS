
#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdarg.h>


extern int syscall2(int, int, int);

int open(const char* filename, int flags) {
    return syscall2(SYS_open, (int) filename, flags);
}

