
#include <sys/syscall.h>
#include <unistd.h>


extern int syscall1(int, int);

int chdir(const char* path) {
    return syscall1(SYS_chdir, (int) path);
}

