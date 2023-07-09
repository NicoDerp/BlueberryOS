
#include <stdlib.h>
#include <bits/memory.h>

#if defined(__is_libk)
#include <kernel/memory.h>
#else
#include <sys/mman.h>
#endif

tag_t* freePages[MEMORY_TOT_EXP];
int completePages[MEMORY_TOT_EXP];
int initialized = 0;


static inline int getIndex(unsigned int num) {

    if (num < (1 << MEMORY_MIN_EXP)) {
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
    return mmap(NULL, count*FRAME_4KB, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
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
    uint32_t index = getIndex(size);

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

        // TODO in usermode this is mmap
        tag = (tag_t*) allocateFrames(pages);

        tag->magic = MEMORY_TAG_MAGIC;
        tag->realsize = pages * FRAME_SIZE;
        tag->index = -1;

        tag->splitprev = NULL;
        tag->splitnext = NULL;

    } else {

        if (tag->magic != MEMORY_TAG_MAGIC) {
            ERROR("Tag (0x%x) (%d) magic has been overwritten!\n", tag, tag->index);
            for (;;) {}
        }

        // Check if tag is the first in the list
        if (freePages[tag->index] == tag)
            freePages[tag->index] = tag->next;

        if  (tag->prev != NULL)
            tag->prev->next = tag->next;

        if (tag->next != NULL)
            tag->next->prev = tag->prev;

        tag->index = -1;
    }

    tag->prev = NULL;
    tag->next = NULL;
    tag->size = size;

    // Check if there is more space left in tag, in that case we split the tag
    int remainder = tag->realsize - size - 2*sizeof(tag_t); // Both this tag and next tag

    // Needs to be more than minimum to split
    if (remainder > (1 << MEMORY_MIN_EXP)) {

        VERBOSE("kmalloc: Splitting tag with size %d with remainder %s\n", size, remainder);

        tag_t* splitTag = (tag_t*) ((uint32_t) tag + sizeof(tag_t) + tag->size);
        splitTag->magic = MEMORY_TAG_MAGIC;

        splitTag->realsize = remainder;
        tag->realsize -= remainder;

        splitTag->next = NULL;
        splitTag->prev = NULL;

        splitTag->splitprev = tag;
        splitTag->splitnext = tag->splitnext;

        tag->splitnext = splitTag;

        if (splitTag->splitnext != NULL)
            splitTag->splitnext->splitprev = splitTag;

        // Insert split tag at beginning
        uint32_t splitIndex = getIndex(remainder - sizeof(tag_t));
        if (freePages[splitIndex] == NULL) {
            freePages[splitIndex] = splitTag;
        } else {
            freePages[splitIndex]->prev = splitTag;
            freePages[splitIndex] = splitTag;
        }
        splitTag->index = splitIndex;
        VERBOSE("Split tag at 0x%x with index %d and size %d\n", splitTag, splitTag->index, remainder - sizeof(tag_t));
    }

    VERBOSE("kmalloc: Returning tag at 0x%x with index %d\n", tag, tag->index);

    return (void*) ((uint32_t) tag + sizeof(tag_t));
}

