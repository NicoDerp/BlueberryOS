
#include <sys/syscall.h>
#include <unistd.h>
#include <bits/types/struct_DIR.h>
#include <stdbool.h>


extern int syscall1(int, int);

__attribute__((__nonnull__))
int closedir (DIR* dir) {
    int status = syscall1(SYS_close, dir->dd_fd);
    dir->dd_used = false;
    return status;
}

