
#include <sys/wait.h>

#include <sys/syscall.h>
#include <unistd.h>


pid_t wait(int* status) {
    return syscall(SYS_waitpid, status);
}

