
#ifndef _BITS_MEMORY_H
#define _BITS_MEMORY_H 1

#include <sys/cdefs.h>
#include <stdint.h>
#include <stddef.h>


#define FRAME_1KB ((unsigned int) 0x400)
#define FRAME_4KB ((unsigned int) 0x1000)

#define FRAME_1MB ((unsigned int) 0x100000)
#define FRAME_4MB ((unsigned int) 0x400000)

#define FRAME_SIZE FRAME_4KB

#define MEMORY_TAG_MAGIC    0xFEAB0CBD
#define MEMORY_MAX_COMPLETE 4
#define MEMORY_MAX_EXP      32
#define MEMORY_MIN_EXP      4
#define MEMORY_TOT_EXP      (MEMORY_MAX_EXP - MEMORY_MIN_EXP)
#define MEMORY_MIN_FRAMES   8

struct tag;
typedef struct tag {
    uint32_t magic;
    uint32_t size;
    uint32_t realsize;
    int index;

    struct tag* prev;
    struct tag* next;

    struct tag* splitprev;
    struct tag* splitnext;
} tag_t;

extern tag_t* freePages[MEMORY_TOT_EXP];
extern int completePages[MEMORY_TOT_EXP];

#if !defined(__is_libk)
#define VERBOSE(format, ...)
#define ERROR(format, ...)
#endif

#endif

