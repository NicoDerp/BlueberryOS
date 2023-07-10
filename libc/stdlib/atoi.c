
#include <stdlib.h>


int atoi(const char* str) {

    unsigned int i = 0;
    int neg = (str[0] == '-');
    if (neg)
        i++;

    int num = 0;
    for (; '0' <= str[i] && str[i] <= '9'; i++) {
        num *= 10;
        num += str[i] - '0';
    }

    return num;
}

