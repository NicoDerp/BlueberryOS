
#include <shadow.h>
#include <sys/syscall.h>
#include <unistd.h>


extern int syscall5(int, int, int, int, int, int);

int getspnam_r(const char* name, struct spwd* spbuf, char* buf, size_t buflen, struct spwd** spbufp) {
    return syscall5(SYS_getspnamr, (int) name, (int) spbuf, (int) buf, buflen, (int) spbufp);
}

