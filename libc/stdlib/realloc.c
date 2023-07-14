
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
#define ERROR(format, ...) {printf("Error: "format, ## __VA_ARGS__);}
#endif

/*
#if !defined(__is_libk)
#define VERBOSE(format, ...) {printf("\e[f;0m[INFO]\e[0m "format, ## __VA_ARGS__);}
#define ERROR(format, ...) {printf("ERROR!J "format, ## __VA_ARGS__);}
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

    //return exp-1;
    return exp - MEMORY_MIN_EXP;
}


#if defined(__is_libk)
void* krealloc(void* ptr, size_t size) {
#else
void* realloc(void* ptr, size_t size) {
#endif

    if (ptr == NULL) {
#if defined(__is_libk)
        return kmalloc(size);
#else
        return malloc(size);
#endif
    }

    // TODO check if tag can absorb right

    tag_t* tag = (tag_t*) ((uint32_t) ptr - sizeof(tag_t));
    if (tag->magic != MEMORY_TAG_MAGIC) {
        ERROR("free: Tag at 0x%x has been corrupted!\n", ptr);
        return ptr;
    }

    // TODO if it is much smaller then split tag as in free
    if (size <= tag->realsize - sizeof(tag_t)) {
        tag->size = size;
        return ptr;
    }

#if defined(__is_libk)
    void* new = kmalloc(size);
#else
    void* new = malloc(size);
#endif

    tag_t* nt = (tag_t*) ((uint32_t) new - sizeof(tag_t));
    memcpy(new, ptr, tag->size);

#if defined(__is_libk)
    kfree(ptr);
#else
    free(ptr);
#endif

    return new;
}


