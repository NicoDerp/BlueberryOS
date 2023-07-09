
#include <sys/mman.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


void test(void) {
    int n = 5;

    int* ptr = mmap(NULL, n*sizeof(int), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

    if (ptr == MAP_FAILED) {
        printf("Mapping failed\n");
        exit(1);
    }

    int err = munmap(ptr, 10*sizeof(int));
    if (err != 0) {
        printf("Unmapping failed\n");
        exit(1);
    }
}

void main() {

    test();
    test();

}



