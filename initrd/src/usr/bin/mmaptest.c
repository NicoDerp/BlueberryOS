
#include <sys/mman.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


void test(void) {

    int n = 5;
    int* ptr1 = (int*) malloc(n*sizeof(int));
    printf("Pointer at 0x%x\n", ptr1);

    int* ptr2 = (int*) malloc(n*sizeof(int));
    printf("Pointer at 0x%x\n", ptr2);

    free(ptr1);
    free(ptr2);
}

void main() {

    test();
    test();

}



