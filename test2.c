
#include <stdio.h>
#include <stdint.h>

typedef struct {
    uint32_t present:1;
    uint32_t readwrite:1;
    uint32_t usermode:1;
    uint32_t reserved:2;
    uint32_t available:3;
    uint32_t address:20;
    uint32_t ooga;
} pagetable_t;

int main(void) {
    long unsigned int num = 0xFFFFFFFFFFFFFFFF;
    pagetable_t* x = (pagetable_t*) &num;
    pagetable_t table = *x;
    printf("0x%x\n", table.address);
}

