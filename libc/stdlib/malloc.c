
#include <stdlib.h>

#if defined(__is_libk)
#include <kernel/logging.h>
#include <kernel/memory.h>
#else
#include <sys/mman.h>
#endif

#include <bits/memory.h>

#include <stdio.h>

tag_t* freePages[MEMORY_TOT_EXP];
int completePages[MEMORY_TOT_EXP];
int initialized = 0;

#if !defined(__is_libk)
#define VERBOSE(format, ...)
#define ERROR(format, ...) {printf("Error: "format, ## __VA_ARGS__);}
#endif

/*
#if !defined(__is_libk)
#define VERBOSE(format, ...) {printf("\e[f;0m[INFO]\e[0m "format, ## __VA_ARGS__);}
#define ERROR(format, ...) {printf("Error: "format, ## __VA_ARGS__);}
#endif
*/


static inline int getIndex(unsigned int num) {

    if (num <= (1 << MEMORY_MIN_EXP)) {
        return 0;
    }

    unsigned int exp = 0;
    while (num >>= 1)
        exp++;

    //return exp-1;
    return exp - MEMORY_MIN_EXP;
}


#if defined(__is_libk)

static inline void* allocateFrames(unsigned int count) {
    return kalloc_frames(count);
}

#else

static inline void* allocateFrames(unsigned int count) {
    VERBOSE("Allocating %d frames\n", count);
    void* ptr = mmap(NULL, count*FRAME_4KB, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    VERBOSE("Got ptr: 0x%x\n", ptr);
    return ptr;
}

#endif


#if defined(__is_libk)
void* kmalloc(size_t size) {
#else
void* malloc(size_t size) {
#endif

    if (initialized == 0) {

        for (size_t i = 0; i < MEMORY_TOT_EXP; i++) {
            freePages[i] = NULL;
            completePages[i] = 0;
        }

        initialized = 1;
    }

    tag_t* tag;
    int index = getIndex(size);

    uint32_t i;
    for (i = index; i < MEMORY_TOT_EXP; i++) {
        tag = freePages[i];
        while ((tag != NULL) && (size + sizeof(tag_t) > tag->realsize)) {
            tag = tag->next;
        }

        if (tag != NULL)
            break;
    }

    if (tag == NULL) {
        uint32_t realsize = size + sizeof(tag_t);
        uint32_t pages = realsize / FRAME_SIZE;
        if ((realsize & (FRAME_SIZE-1)) != 0)
            pages++;

        if (pages < MEMORY_MIN_FRAMES)
            pages = MEMORY_MIN_FRAMES;

        tag = (tag_t*) allocateFrames(pages);

        tag->magic = MEMORY_TAG_MAGIC;
        tag->realsize = pages * FRAME_SIZE;

        tag->splitprev = NULL;
        tag->splitnext = NULL;

    } else {

        if (tag->magic != MEMORY_TAG_MAGIC) {
            ERROR("malloc: Tag (0x%x) (%d) magic has been overwritten!\n", tag, tag->index);
            for (;;) {}
        }

        // Check if tag is the first in the list
        if (freePages[tag->index] == tag)
            freePages[tag->index] = tag->next;

        if  (tag->prev != NULL)
            tag->prev->next = tag->next;

        if (tag->next != NULL)
            tag->next->prev = tag->prev;

        if ((tag->splitprev == NULL) && (tag->splitnext == NULL))
            completePages[index]--;
    }

    tag->prev = NULL;
    tag->next = NULL;
    tag->size = size;
    tag->index = -1;

    // Check if there is more space left in tag, in that case we split the tag
    int remainder = tag->realsize - size - sizeof(tag_t);
    int* magic = (int*) (0x400295 - sizeof(tag_t));

    // Needs to be more than minimum to split
    if (remainder - sizeof(tag_t) > (1 << MEMORY_MIN_EXP)) {

        VERBOSE("kmalloc: Splitting tag with size %d with remainder %d\n", size, remainder);

#if !defined(__is_libk)
        printf("Malloc Number: 0x%x\n", *magic);
#endif

        tag_t* splitTag = (tag_t*) ((uint32_t) tag + sizeof(tag_t) + tag->size);
        uint32_t splitIndex = getIndex(remainder - sizeof(tag_t));
        splitTag->magic = MEMORY_TAG_MAGIC;
        splitTag->index = splitIndex;

        splitTag->realsize = remainder;
        tag->realsize -= remainder;

        splitTag->next = NULL;
        splitTag->prev = NULL;

        splitTag->splitprev = tag;
#if !defined(__is_libk)
        printf("Malloc Number: 0x%x\n", *magic);
#endif
        splitTag->splitnext = tag->splitnext;
#if !defined(__is_libk)
        printf("Malloc Number: 0x%x\n", *magic);
        printf("Malloc Number: 0x%x\n", (unsigned int) magic);
        printf("Malloc Number: 0x%x\n", (unsigned int) &splitTag->splitnext);
#endif

        tag->splitnext = splitTag;

        if (splitTag->splitnext != NULL)
            splitTag->splitnext->splitprev = splitTag;

        // Insert split tag at beginning
        if (freePages[splitIndex] == NULL) {
            freePages[splitIndex] = splitTag;
        } else {
            splitTag->next = freePages[splitIndex];
            freePages[splitIndex]->prev = splitTag;
            freePages[splitIndex] = splitTag;
        }
        VERBOSE("Split tag at 0x%x with index %d and size %d\n", splitTag, splitTag->index, remainder - sizeof(tag_t));
    }
#if !defined(__is_libk)
    printf("Malloc Number: 0x%x\n", *magic);
#endif

    VERBOSE("kmalloc: Returning tag at 0x%x with index %d\n", tag, tag->index);

    return (void*) ((uint32_t) tag + sizeof(tag_t));
}

