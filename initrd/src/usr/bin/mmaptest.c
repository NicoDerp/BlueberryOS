
#include <sys/mman.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


void test(void) {

    int n = 5;
    int* ptr1 = (int*) malloc(n*sizeof(int));
    printf("Pointer at 0x%x\n", (unsigned int) ptr1);

    int* ptr2 = (int*) malloc(n*sizeof(int));
    printf("Pointer at 0x%x\n", (unsigned int) ptr2);
    memset(ptr2, 12891, n*sizeof(int));

    printf("Freeing\n");
    free(ptr1);
    printf("Freeing\n");
    free(ptr2);
    printf("\n");
}

int main() {

    char* ptr = (char*) malloc(64);
    memset(ptr, 0, 64);
    ptr = realloc(ptr, 128);
    memset(ptr, 0, 128);
    ptr = realloc(ptr, 256);
    memset(ptr, 0, 256);
    ptr = realloc(ptr, 512);
    memset(ptr, 0, 512);
    free(ptr);

    test();
    test();

    return 0;
}



