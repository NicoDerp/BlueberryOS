
#include <sys/syscall.h>
#include <unistd.h>


extern int syscall0(int);

pid_t fork(void) {
    return syscall0(SYS_fork);
}

