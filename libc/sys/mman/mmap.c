
#include <sys/mman.h>
#include <sys/syscall.h>
#include <stddef.h>
#include <stdint.h>


extern int syscall6(int, int, int, int, int, int, int);

void* mmap(void* addr, size_t length, int prot, int flags, int fd, uint32_t offset) {
    return (void*) syscall6(SYS_mmap, (int) addr, length, prot, flags, fd, offset);
}

