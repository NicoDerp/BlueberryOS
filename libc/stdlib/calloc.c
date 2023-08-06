
#include <stdlib.h>
#include <bits/memory.h>
#include <string.h>

#include <stdio.h>

#if defined(__is_libk)
#include <kernel/logging.h>
#include <kernel/memory.h>
#endif



#if !defined(__is_libk)
#define VERBOSE(format, ...)
#define ERROR(format, ...) //printf("Error: "format, ## __VA_ARGS__)
#endif

/*
#if !defined(__is_libk)
#define VERBOSE(format, ...) {//printf("\e[f;0m[INFO]\e[0m "format, ## __VA_ARGS__);}
#define ERROR(format, ...) {//printf("ERROR!J "format, ## __VA_ARGS__);}
#endif
*/


static inline int getIndex(unsigned int num) {

    if (num <= (1 << MEMORY_MIN_EXP)) {
        //return MEMORY_MIN_EXP-1;
        return 0;
    }

    unsigned int exp = 0;
    while (num >>= 1)
        exp++;

    return exp - MEMORY_MIN_EXP;
}


#if defined(__is_libk)
void* kcalloc(size_t items, size_t size) {
#else
void* calloc(size_t items, size_t size) {
#endif

#if defined(__is_libk)
    void* ptr = kmalloc(items * size);
#else
    void* ptr = malloc(items * size);
#endif

    memset(ptr, 0, items*size);

    return ptr;
}


