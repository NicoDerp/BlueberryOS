
#include <sys/syscall.h>
#include <unistd.h>


extern int syscall0(int);

void yield(void) {
    syscall0(SYS_yield);
}

