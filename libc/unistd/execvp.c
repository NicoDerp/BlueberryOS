
#include <sys/syscall.h>
#include <unistd.h>


int execvp(const char* file, char* const argv[]) {
    return syscall(SYS_execvp, file, argv);
}

