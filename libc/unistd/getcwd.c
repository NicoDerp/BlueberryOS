
#include <sys/syscall.h>
#include <unistd.h>
#include <stdarg.h>


extern int syscall2(int, int, int);

char* getcwd(char* buf, size_t size) {
    return (char*) syscall2(SYS_getcwd, (int) buf, size);
}

