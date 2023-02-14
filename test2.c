
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int main(void) {
    char buf[64];
    int a = 120;
    itoa(a, buf, 10);
    printf("AA: %s\n", buf);
    return 0;
}

