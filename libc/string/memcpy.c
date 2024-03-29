
#include <string.h>

void* memcpy(void* __restrict dstptr, const void* __restrict srcptr, size_t size) {
    unsigned char* dst = (unsigned char*) dstptr;
    const unsigned char* src = (const unsigned char*) srcptr;

    if (srcptr == dstptr)
        return dstptr;

    for (size_t i = 0; i < size; i++) {
        dst[i] = src[i];
    }

    return dstptr;
}

