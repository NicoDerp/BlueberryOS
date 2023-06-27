
#include <sys/syscall.h>
#include <unistd.h>


extern int syscall2(int, int, int);

int execvp(const char* file, char* const argv[]) {
    return syscall2(SYS_execvp, (int) file, (int) argv);
}

