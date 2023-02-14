
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

char* itoa(int value, char* sp, int base) {
    char tmp[64];
    tmp[63] = '\0';
    int i = 62;

    bool neg = value < 0;
    if (neg) {
        value = -value;
    }

    while (value != 0) {
        char c = value % base + '0';
        value /= base;

        if (i == 0) {
            printf("Buffer overflow 1 in libc/stdlib/itoa.c\n");
        }

        tmp[i] = c;
        i--;
    }

    if (i <= 3) {
        printf("Buffer overflow 2 in libc/stdlib/itoa.c\n");
    }

    if (base == 16) {
        tmp[i] = 'x';
        i--;
        tmp[i] = '0';
        i--;
    }

    if (neg) {
        tmp[i] = '-';
        i--;
    }

    char* tmp2 = &tmp[i+1];
    while (*tmp2 != 0) {
        *sp = *tmp2;
        sp++;
        tmp2++;
    }

    sp--;
    return sp;
}

