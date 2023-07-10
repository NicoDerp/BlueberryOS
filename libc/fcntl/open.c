
#include <fcntl.h>
#include <sys/syscall.h>
#include <stdarg.h>


extern int syscall3(int, int, int, int);

int open(const char* filename, int flags, ...) {
    if (flags & O_CREAT) {

        va_list args;
        va_start(args, flags);

        int permissions = (int) va_arg(args, int);
        int ret = syscall3(SYS_open, (int) filename, flags, permissions);

        va_end(args);

        return ret;
    }

    return syscall3(SYS_open, (int) filename, flags, 0);
}

