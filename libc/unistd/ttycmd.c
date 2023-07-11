
#include <sys/syscall.h>
#include <unistd.h>


extern int syscall3(int, int, int, int);

int ttycmd(int cmd, int* args, unsigned int** ret) {
    return syscall3(SYS_ttycmd, cmd, (int) args, (int) ret);
}

