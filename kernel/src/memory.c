
#include <kernel/logging.h>
#include <kernel/memory.h>
#include <kernel/errors.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <stdio.h>

void kalloc_cache();

char* frame_map;
pageframe_t cached_frame_map[FRAME_CACHE_SIZE];
uint32_t totalFrameMapSize;
uint32_t frameMapSize;

uint32_t framestart;
size_t cachedIndex = 0;

uint32_t framesUsed = 0;

void memory_initialize(uint32_t framestart_, uint32_t bytes) {

    frame_map = (char*) framestart_;

    totalFrameMapSize = bytes / FRAME_SIZE;

    // Minimum requirement, increase if needed
    frameMapSize = 2*FRAME_4MB;
    framestart = framestart_ + frameMapSize/8;

    cachedIndex = 0;
    kalloc_cache();
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
        if (index >= frameMapSize) {
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
    if (index >= frameMapSize) {
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
    if (index + count >= frameMapSize) {
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
            if (index >= frameMapSize) {
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
    for (size_t i = 0; i < frameMapSize; i++) {
        if ((frame_map[i >> 3] & (1 << (i & 0x7))) == 1) {
            count++;
        }
    }

    return count;
    */

    return framesUsed * FRAME_SIZE;
}

