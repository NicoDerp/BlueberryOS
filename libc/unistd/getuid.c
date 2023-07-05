
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>


extern int syscall0(int);

uid_t getuid(void) {
    return syscall0(SYS_getuid);
}

