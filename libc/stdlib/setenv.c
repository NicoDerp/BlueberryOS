
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>


extern int syscall3(int, int, int, int);

int setenv(const char* name, const char* value, int overwrite) {
    return syscall3(SYS_setenv, (int) name, (int) value, overwrite);
}

