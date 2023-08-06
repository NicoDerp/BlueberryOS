
#include <string.h>


char* strcat(char* dst, const char* src) {
    
    unsigned int dstlen = strlen(dst);
    unsigned int i;
    for (i = 0; src[i]; i++) {
        dst[dstlen + i] = src[i];
    }
    dst[dstlen + i] = '\0';

    return dst;
}

