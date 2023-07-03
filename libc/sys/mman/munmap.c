
#include <sys/mman.h>
#include <sys/syscall.h>
#include <stddef.h>
#include <stdint.h>


extern int syscall2(int, int, int);

int munmap(void* addr, size_t length) {
    return syscall2(SYS_munmap, (int) addr, length);
}

