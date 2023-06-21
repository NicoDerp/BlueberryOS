
#include <sys/syscall.h>
#include <unistd.h>

void yield(void) {
    syscall(SYS_yield);
}

