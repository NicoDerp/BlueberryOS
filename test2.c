
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

uint8_t gdt[5][8];

int main(void) {
    printf("OOga: %ld\n", sizeof(gdt));
    return 0;
}

