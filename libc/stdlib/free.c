
#include <stdlib.h>
#include <bits/memory.h>

#include <stdio.h>

#if defined(__is_libk)
#include <kernel/logging.h>
#include <kernel/memory.h>
#else
#include <sys/mman.h>
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

    tag_t* tag;

    if (ptr == NULL)
        return;

    tag = (tag_t*) ((uint32_t) ptr - sizeof(tag_t));
    if (tag->magic != MEMORY_TAG_MAGIC) {
        ERROR("free: Tag at 0x%x has been corrupted!\n", ptr);
        return;
    }

    //int oldIndex = getIndex(tag->realsize - sizeof(tag_t));

    // Merge with previous free tags
    while ((tag->splitprev != NULL) && (tag->splitprev->index >= 0)) {

        VERBOSE("free: Merging with left\n");
        tag_t* left = tag->splitprev;
        if (left->magic != MEMORY_TAG_MAGIC) {
            ERROR("free: Left tag at 0x%x has been corrupted, which means a previous allocated memory has overwritten their memory!\n", left);
            return;
        }

        left->realsize += tag->realsize;

        left->splitnext = tag->splitnext;
        if (tag->splitnext != NULL) {
            if (tag->splitnext->magic != MEMORY_TAG_MAGIC) {
                ERROR("free: Splitnext tag at 0x%x has been corrupted!\n", tag->splitnext);
                return;
            }

            tag->splitnext->splitprev = left;
        }

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

        VERBOSE("free: Absorbing right\n");
        tag_t* right = tag->splitnext;
        if (right->magic != MEMORY_TAG_MAGIC) {
            ERROR("free: Right tag at 0x%x has been corrupted, which means program has overwritten their memory!\n", right);
            return;
        }

        tag->realsize += right->realsize;

        tag->splitnext = right->splitnext;
        if (right->splitnext != NULL) {
            if (right->splitnext->magic != MEMORY_TAG_MAGIC) {
                ERROR("free: Splitnext tag at 0x%x has been corrupted!\n", right->splitnext);
                return;
            }

            right->splitnext->splitprev = tag;
        }

        // Check if tag is the first in the list
        if (freePages[right->index] == right)
            freePages[right->index] = right->next;

        if (right->prev != NULL)
            right->prev->next = right->next;

        if (right->next != NULL)
            right->next->prev = right->prev;

        right->prev = NULL;
        right->next = NULL;
        right->index = -1;
    }

    int index = getIndex(tag->realsize - sizeof(tag_t));
    VERBOSE("Tag has size %d at index %d\n", tag->realsize, index);

    // Remove tag from old index and cut ties if its size has changed
    if (tag->index != index) {

        tag->index = index;

        if (freePages[tag->index] == tag)
            freePages[tag->index] = tag->next;

        if (tag->prev != NULL)
            tag->prev->next = tag->next;

        if (tag->next != NULL)
            tag->next->prev = tag->prev;

        // Insert tag at start of new location
        if (freePages[index] == NULL) {
            freePages[index] = tag;
        } else {
            tag->next = freePages[index];
            freePages[index]->prev = tag;
            freePages[index] = tag;
        }
    }

    tag->prev = NULL;
    tag->next = NULL;

    // This needs to be done at the end because if we actually free then we can't access tag anymore
    if (tag->splitprev == NULL && tag->splitnext == NULL) {        

        if (completePages[index] >= MEMORY_MAX_COMPLETE) {
            //freePages[index] = NULL;

            uint32_t pages = tag->realsize / FRAME_SIZE;
            if ((tag->realsize & (FRAME_SIZE-1)) != 0)
                pages++;

            if (pages < MEMORY_MIN_FRAMES)
                pages = MEMORY_MIN_FRAMES;

            freeFrames((void*) tag, pages);
            return;
        }

        completePages[index]++;
    }
}


