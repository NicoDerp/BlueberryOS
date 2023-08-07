
#include <stdio.h>


int main() {

    unsigned char num = 255;
    unsigned char* n = &num;
    printf("Num: %u\n", (unsigned char) (*n));

    return 0;
}

