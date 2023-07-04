
#include <grp.h>
#include <sys/syscall.h>
#include <sys/types.h>


extern int syscall5(int, int, int, int, int, int);

int getgrgid_r(gid_t gid, struct group* grp, char* buffer, size_t bufsize, struct group** result) {
    return syscall5(SYS_getgrgidr, gid, (int) grp, (int) buffer, bufsize, (int) result);
}

