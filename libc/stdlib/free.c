
#include <stdlib.h>
#include <bits/memory.h>

#if defined(__is_libk)
#include <kernel/memory.h>
#else
#include <sys/mman.h>
#endif


static inline int getIndex(unsigned int num) {

    if (num < (1 << MEMORY_MIN_EXP)) {
        ERROR("Got under minimum exponent\n");
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

inline void freeFrames(void* frame, unsigned int count) {
    kfree_frames(frame, count);
}

#else
inline void freeFrames(void* frame, unsigned int count) {
    munmap(frame, count);
}

#endif


#if defined(__is_libk)
void kfree(void* ptr) {
#else
void free(void* ptr) {
#endif

    if (ptr == NULL)
        return;

    tag_t* tag = (tag_t*) ((uint32_t) ptr - sizeof(tag_t));
    if (tag->magic != MEMORY_TAG_MAGIC) {
        ERROR("kfree: Tag at 0x%x has been corrupted!\n", ptr);
        return;
    }

    // Merge with previous
    while ((tag->splitprev != NULL) && (tag->splitprev->index >= 0)) {

        tag_t* left = tag->splitprev;

        left->realsize += tag->realsize;

        left->splitnext = tag->splitnext;
        if (tag->splitnext != NULL)
            tag->splitnext->splitprev = left;

        // Check if tag is the first in the list
        if (freePages[tag->index] == tag)
            freePages[tag->index] = tag->next;

        if (tag->prev != NULL)
            tag->prev->next = tag->next;

        if (tag->next != NULL)
            tag->next->prev = tag->prev;

        tag->prev = NULL;
        tag->next = NULL;
        tag->index = -1;

        tag = left;
    }

    // Merge with next
    while ((tag->splitnext != NULL) && (tag->splitnext->index >= 0)) {

        tag_t* right = tag->splitnext;

        tag->realsize += right->realsize;

        tag->splitnext = right->splitnext;
        if (right->splitnext != NULL) {
            right->splitnext->splitprev = tag;
        }

        // Check if tag is the first in the list
        if (freePages[right->index] == right)
            freePages[right->index] = right->next;

        if  (right->prev != NULL)
            right->prev->next = right->next;

        if (right->next != NULL)
            right->next->prev = right->prev;

        right->prev = NULL;
        right->next = NULL;
        right->index = -1;
    }

    uint32_t index = getIndex(tag->realsize - sizeof(tag_t));
    if (tag->prev == NULL && tag->next == NULL) {        

        if (completePages[index] >= MEMORY_MAX_COMPLETE) {
            freePages[index] = NULL;

            uint32_t pages = tag->realsize / FRAME_SIZE;
            freeFrames((void*) tag, pages);
        }

        completePages[index]++;
    }

    // Insert free tag at beginning
    if (freePages[index] == NULL) {
        freePages[index] = tag;
    } else {
        freePages[index]->prev = tag;
        freePages[index] = tag;
    }
}


