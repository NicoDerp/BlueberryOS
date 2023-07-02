
#include <sys/wait.h>
#include <sys/syscall.h>


extern int syscall1(int, int);

pid_t wait(int* status) {
    return syscall1(SYS_waitpid, (int) status);
}

