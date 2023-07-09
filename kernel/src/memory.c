
#include <kernel/logging.h>
#include <kernel/memory.h>
#include <kernel/errors.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <stdio.h>

void kalloc_cache();

char frame_map[FRAME_MAP_SIZE/8];
pageframe_t cached_frame_map[FRAME_CACHE_SIZE];

uint32_t framestart;
size_t cachedIndex = 0;

tag_t* freePages[MEMORY_TOT_EXP];
int completePages[MEMORY_TOT_EXP];

uint32_t framesUsed = 0;

void memory_initialize(uint32_t framestart_, uint32_t bytes) {

    framestart = framestart_;

    // TODO FRAME_MAP_SIZE should be bytes / FRAME_4KB
    (void) bytes;

    cachedIndex = 0;
    kalloc_cache();

    for (size_t i = 0; i < MEMORY_TOT_EXP; i++) {
        freePages[i] = NULL;
        completePages[i] = 0;
    }
}

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

void* kmalloc(size_t size) {

    tag_t* tag;
    uint32_t index = getIndex(size);

    //printf("Index at %d\n", index);

    uint32_t i;
    for (i = index; i < MEMORY_TOT_EXP; i++) {
        tag = freePages[i];
        while ((tag != NULL) && (size + sizeof(tag_t) > tag->realsize)) {
            tag = tag->next;
        }

        if (tag != NULL)
            break;
    }

    if (tag == NULL)
        printf("tag 0x%x\n", tag);
    else
        printf("tag 0x%x at %d (%d) \n", tag, i, tag->index);

    if (tag == NULL) {
        uint32_t realsize = size + sizeof(tag_t);
        uint32_t pages = realsize / FRAME_SIZE;
        if ((realsize & (FRAME_SIZE-1)) != 0)
            pages++;

        if (pages < MEMORY_MIN_FRAMES)
            pages = MEMORY_MIN_FRAMES;

        // TODO in usermode this is mmap
        tag = (tag_t*) kalloc_frames(pages);

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

        printf("Magic: 0x%x\n", tag->magic);
        printf("ajed index: %d\n", tag->index);
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

    printf("Tag at 0x%x size %d\n", tag, size);

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
        printf("Split tag at 0x%x with index %d and size %d\n", splitTag, splitTag->index, remainder - sizeof(tag_t));
        /*
        printf("Split magic: 0x%x\n", splitTag->magic);
        */
    }

    printf("Tag is at 0x%x with index %d\n", tag, tag->index);
    /*
    printf("Returning 0x%x for size %d\n", (uint32_t) tag + sizeof(tag_t), size);
    printf("allocMagic: 0x%x\n", tag->magic);
    */

    return (void*) ((uint32_t) tag + sizeof(tag_t));
}

void kfree(void* ptr) {

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
            kfree_frames((void*) tag, pages);
        }

        completePages[index]++;
    }

    printf("realsiez: %d\n", tag->realsize);
    printf("free index: %d, freePages[i]: 0x%x\n", index, freePages[index]);

    // Insert free tag at beginning
    if (freePages[index] == NULL) {
        freePages[index] = tag;
    } else {
        freePages[index]->prev = tag;
        freePages[index] = tag;
    }
}

pageframe_t kalloc_frame(void) {
    if (cachedIndex >= FRAME_CACHE_SIZE) {
        cachedIndex = 0;
        kalloc_cache();
    }

    pageframe_t frame = cached_frame_map[cachedIndex];
    /*
    if ((unsigned int) frame >= (0xC03FF000-FRAME_4KB)) {
        kerror("Allocated frame goes into framebuffer\n");
    }
    */

    VERBOSE("kalloc_frame: Allocated 4KB frame at 0x%x\n", frame);

    cachedIndex++;

    return frame;
}

pageframe_t kalloc_frames(unsigned int count) {

    if (count == 0) {
        VERBOSE("kalloc_frames: Was called with count 0 so returning NULL\n");
        for (;;) {}
        return (pageframe_t) 0;
    }

    if (count == 1) {
        VERBOSE("kalloc_frames: Redirecting to more efficient kalloc_frame\n");
        return kalloc_frame();
    }

    size_t startindex = 0;
    size_t index = 0;
    size_t cons = 0;
    while (cons != count) {
        if ((frame_map[index >> 3] & (1 << (index & 0x7))) == 0) {
            
            if (cons == 0) {
                startindex = index;
            }
            cons++;

        } else {

            cons = 0;

        }

        index++;
        if (index >= FRAME_MAP_SIZE) {
            FATAL("Out of memory!\n");
            kabort();
            return (pageframe_t) -1;
        }
    }

    for (size_t i = startindex; i < startindex + count; i++) {
        frame_map[i >> 3] |= (1 << (i & 0x7));
    }

    pageframe_t frame = (pageframe_t) (startindex*FRAME_SIZE + framestart);
    framesUsed += count;

    VERBOSE("kalloc_frames: Allocated %d 4KB frames starting at 0x%x, ending at 0x%x\n", count, frame, frame + FRAME_4KB*count);

    return frame;
}

void kfree_frame(pageframe_t frame) {

    VERBOSE("kfree_frame: Freeing frame at 0x%x\n", frame);

    if (((uint32_t) frame & (FRAME_SIZE-1)) != 0) {
        ERROR("Kernel tried to free non 4KB aligned-pageframe\n");
    }
    
    size_t index = (unsigned int) frame - framestart;
    if (index != 0) {
        index /= FRAME_SIZE;
    }
    if (index >= FRAME_MAP_SIZE) {
        ERROR("Index not in correct range: 0x%x from frame 0x%x\n", index, frame);
        return;
    }

    if ((frame_map[index >> 3] & (1 << (index & 0x7))) == 0) {
        ERROR("Frame at 0x%x freed multiple times!\n", frame);
    }
    frame_map[index >> 3] &= ~(1 << (index & 0x7));
    framesUsed--;
}

void kfree_frames(pageframe_t frame, unsigned int count) {

    if (count == 0)
        return;

    VERBOSE("kfree_frames: Freeing %d frames starting at 0x%x\n", count, frame);

    if (((uint32_t) frame & (FRAME_SIZE-1)) != 0) {
        ERROR("Kernel tried to free non 4KB aligned-pageframe\n");
    }
    
    size_t index = (unsigned int) frame - framestart;
    if (index != 0) {
        index /= FRAME_SIZE;
    }
    if (index + count >= FRAME_MAP_SIZE) {
        ERROR("Index not in correct range: 0x%x from frame 0x%x\n", index, frame);
        return;
    }

    if ((frame_map[index >> 3] & (1 << (index & 0x7))) == 0) {
        ERROR("Frame at 0x%x freed multiple times!\n", frame);
    }

    for (size_t i = 0; i < count; i++) {
        frame_map[(index+i) >> 3] &= ~(1 << ((index+i) & 0x7));
    }
    framesUsed -= count;
}

void kalloc_cache(void) {
    size_t index = 0;

    for (size_t c = 0; c < FRAME_CACHE_SIZE; c++) {

        // Check for first free bit
        while ((frame_map[index >> 3] & (1 << (index & 0x7))) != 0) {
            index++;
            if (index >= FRAME_MAP_SIZE) {
                FATAL("Out of memory!\n");
                kabort();
                return;
            }
        }

        frame_map[index >> 3] |= (1 << (index & 0x7));
        cached_frame_map[c] = (pageframe_t) (index*FRAME_SIZE + framestart);
    }

    framesUsed += FRAME_CACHE_SIZE;
}

uint32_t get_used_memory(void) {

    /*
    uint32_t count = 0;
    for (size_t i = 0; i < FRAME_MAP_SIZE; i++) {
        if ((frame_map[i >> 3] & (1 << (i & 0x7))) == 1) {
            count++;
        }
    }

    return count;
    */

    return framesUsed * FRAME_SIZE;
}

