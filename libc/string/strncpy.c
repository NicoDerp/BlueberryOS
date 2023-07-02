
#include <string.h>


char* strncpy(char* dest, const char* src, size_t n) {

    size_t i = 0;
    while (src[i] != 0) {
        if (i >= n)
            return dest;

        dest[i] = src[i];
        i++;
    }

    dest[i] = '\0';

    return dest;
}


