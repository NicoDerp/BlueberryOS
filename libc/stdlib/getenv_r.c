
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>


extern int syscall3(int, int, int, int);

int getenv_r(const char* name, char* out, size_t size) {
    return syscall3(SYS_getenv, (int) name, (int) out, (int) size);
}

