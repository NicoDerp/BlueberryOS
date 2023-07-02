
#include <sys/stat.h>
#include <sys/syscall.h>


extern int syscall2(int, int, int);

int lstat(const char* __restrict pathname, struct stat* __restrict statbuf) {
    return syscall2(SYS_lstat, (int) pathname, (int) statbuf);
}

