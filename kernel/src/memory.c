
#include <kernel/logging.h>
#include <kernel/memory.h>
#include <kernel/paging.h>
#include <kernel/usermode.h>
#include <kernel/errors.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <stdio.h>

void kalloc_cache();

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

char* frame_map;
pageframe_t cached_frame_map[MAX_FRAME_CACHE_SIZE];
uint32_t totalFrameMapSize = 0;
uint32_t frameMapSize = 0;
uint32_t frame_cache_size = 0;

uint32_t framestart;
size_t cachedIndex = 0;

uint32_t framesUsed;

void mapPageframe(pageframe_t pf) {

    map_page(v_to_p((unsigned int) pf), (unsigned int) pf, true, true);

    for (unsigned int i = 0; i < PROCESSES_MAX; i++) {
        process_t* process = &processes[i];
        if (!process->initialized)
            return;

        map_page_pd(process->pd, v_to_p((unsigned int) pf), (unsigned int) pf, true, true);
    }

    // Security measure so we don't have enough frames for pagetables
    pageframe_t npf = (pageframe_t) ((unsigned int) pf + FRAME_SIZE);
    map_page(v_to_p((unsigned int) npf), (unsigned int) npf, true, true);

    for (unsigned int i = 0; i < PROCESSES_MAX; i++) {
        process_t* process = &processes[i];
        if (!process->initialized)
            return;

        map_page_pd(process->pd, v_to_p((unsigned int) npf), (unsigned int) npf, true, true);
    }
}


void memory_initialize(uint32_t total, uint32_t frames) {

    frame_map = (char*) FRAME_START;
    totalFrameMapSize = total / FRAME_SIZE;

    // Minimum requirement, increase if needed
    //frameMapSize = MIN(totalFrameMapSize, 600);
    frameMapSize = totalFrameMapSize;

    uint32_t offset = frameMapSize / 8;
    if (frameMapSize & 7)
        offset++;

    framestart = (FRAME_SIZE - (FRAME_START + offset) % FRAME_SIZE + (FRAME_START + offset));

    framesUsed = 0;
    cachedIndex = 0;
    frame_cache_size = 0;
    kalloc_cache(frames);
}

void memory_mark_allocated(uint32_t start, uint32_t end) {

    if ((start & 0x03FF) || (end & 0x03FF)) {
        FATAL("Memory to mark used isn't page-aligned!\n");
        kabort();
    }

    size_t startindex = (start - framestart) / FRAME_SIZE;
    size_t endindex = (end - framestart) / FRAME_SIZE;

    if (endindex >= frameMapSize) {
        FATAL("End index outside memory map!\n");
        kabort();
    }

    for (size_t index = startindex; index <= endindex; index++) {

        frame_map[index >> 3] |= (1 << (index & 0x7));
        framesUsed++;
    }
}

pageframe_t kalloc_frame(void) {
    if (cachedIndex >= frame_cache_size) {
        cachedIndex = 0;
        kalloc_cache(MAX_FRAME_CACHE_SIZE);
    }

    pageframe_t frame = cached_frame_map[cachedIndex];
    /*
    if ((unsigned int) frame >= (0xC03FF000-FRAME_4KB)) {
        kerror("Allocated frame goes into framebuffer\n");
    }
    */

    VERBOSE("kalloc_frame: Allocated 4KB frame at 0x%x\n", frame);

    /*
    if (loggingEnabled)
        printf("Allocated frame at 0x%x\n", frame);
    */

    cachedIndex++;
    framesUsed++;

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

        uint32_t pf = i * FRAME_SIZE + framestart;
        uint32_t page = getPage(pf);
        if (!(page & 1)) {
            VERBOSE("kalloc_frames: Mapping pageframe p0x%x to c0x%x\n", v_to_p(pf), pf);
            mapPageframe((pageframe_t) pf);
        }
    }

    pageframe_t frame = (pageframe_t) (startindex*FRAME_SIZE + framestart);
    framesUsed += count;

    VERBOSE("kalloc_frames: Allocated %d 4KB frames starting at 0x%x, ending at 0x%x\n", count, frame, frame + FRAME_4KB*count);

    /*
    if (loggingEnabled)
        printf("kalloc_frames: Allocated %d 4KB frames starting at 0x%x, ending at 0x%x\n", count, frame, frame + FRAME_4KB*count);
    */

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

void kalloc_cache(uint32_t cacheSize) {
    size_t index = 0;

    frame_cache_size = 0;
    for (size_t c = 0; c < cacheSize; c++) {

        // Check for first free bit
        while ((frame_map[index >> 3] & (1 << (index & 0x7))) != 0) {
            index++;

            /*
            if (loggingEnabled)
                printf("Checking index %d/%d with %d\n", index+1, frameMapSize, c);
            */

            // Only abort if we don't have any more frames at first cache
            if (index >= frameMapSize) {
                if (c != 0)
                    goto end;

                FATAL("Out of memory!\n");
                kabort();
                return;
            }
        }

        uint32_t pf = index * FRAME_SIZE + framestart;
        frame_map[index >> 3] |= (1 << (index & 0x7));
        cached_frame_map[c] = (pageframe_t) pf;

        uint32_t page = getPage(pf);
        if (!(page & 1)) {
            VERBOSE("kalloc_cache: Mapping pageframe p0x%x to c0x%x\n", v_to_p(pf), pf);
            mapPageframe((pageframe_t) pf);
        }

        frame_cache_size++;
    }

end:
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

