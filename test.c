
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

struct mystruct {

    char ar;
    uint32_t n1;
    uint32_t n2;
    uint32_t n3;
    uint32_t n4;

};

int main ()
{
    printf("size: %ld\n", sizeof(struct mystruct));
    return 0;
}

