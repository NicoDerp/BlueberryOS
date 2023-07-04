
#include <pwd.h>
#include <sys/types.h>
#include <sys/syscall.h>


extern int syscall5(int, int, int, int, int, int);

int getpwuid_r(uid_t uid, struct passwd* pwd, char* buffer, size_t bufsize, struct passwd** result) {
    return syscall5(SYS_getpwuidr, uid, (int) pwd, (int) buffer, bufsize, (int) result);
}


