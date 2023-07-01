
#include <dirent.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <bits/types/struct_DIR.h>
#include <stddef.h>


extern int syscall4(int, int, int, int, int);

int getdirentries(int fd, char* buf, size_t nbytes, uint32_t* restrict basep) {
    return syscall4(SYS_getdirentries, fd, (int) buf, (int) nbytes, (int) basep);
}

