
#include <sys/stat.h>
#include <sys/syscall.h>


extern int syscall2(int, int, int);

int stat(const char* __restrict pathname, struct stat* __restrict statbuf) {
    return syscall2(SYS_stat, (int) pathname, (int) statbuf);
}

