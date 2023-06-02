
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

char* itoa(int value, char* sp, int base) {
    char tmp[64];
    int i = 63;
    tmp[i] = '\0';
    i--;

    bool neg = value < 0;
    if (neg) {
        value = -value;
    }

    do {
        char c = value % base + '0';
        value /= base;

        if (c > '9' && c < 'A') {
            c += 'A' - '9' - 1;
        }

        tmp[i] = c;
        i--;

        if (i == 0) {
            printf("Buffer overflow 1 in libc/stdlib/itoa.c\n");
            break;
        }

    }
    while (value != 0);

    if (i <= 3) {
        printf("Buffer overflow 2 in libc/stdlib/itoa.c\n");
    }

    /*
    if (base == 16) {
        tmp[i] = 'x';
        i--;
        tmp[i] = '0';
        i--;
    }
    */

    if (base == 10 && neg) {
        tmp[i] = '-';
        i--;
    }

    /*
    char* tmp2 = &tmp[i+1];
    while (*tmp2 != 0) {
        *sp = *tmp2;
        sp++;
        tmp2++;
    }

    sp--;
    */

    for (int j = 0; j < 63-i; j++) {
        sp[j] = tmp[i+j+1];
    }

    return sp;
}

