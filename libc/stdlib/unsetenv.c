
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>


extern int syscall3(int, int, int, int);

int unsetenv(const char* name) {
    return syscall3(SYS_setenv, (int) name, (int) NULL, 1);
}

