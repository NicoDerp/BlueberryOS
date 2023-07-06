
#include <sys/syscall.h>
#include <unistd.h>


extern int syscall1(int, int);

int setuid(uid_t uid) {
    return syscall1(SYS_setuid, uid);
}

