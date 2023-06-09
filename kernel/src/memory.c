
#include <kernel/memory.h>
#include <kernel/errors.h>
#include <stddef.h>
#include <stdbool.h>

#include <stdio.h>

pageframe_t kalloc_cachedframe(void);

frame_map_t frame_map[FRAME_MAP_SIZE];
pageframe_t cached_frame_map[FRAME_CACHE_SIZE];

size_t cachedIndex = 0;
bool firsttime = true;

pageframe_t kalloc_frame(void) {
    if (cachedIndex >= FRAME_CACHE_SIZE || firsttime) {
        cachedIndex = 0;
        firsttime = false;
        for (size_t i = 0; i < FRAME_CACHE_SIZE; i++) {
            cached_frame_map[i] = kalloc_cachedframe();
        }
    }

    pageframe_t frame = cached_frame_map[cachedIndex];
    cachedIndex++;

    return frame;
}

void kfree_frame(pageframe_t frame) {
    size_t index = (unsigned int) frame - FRAME_START;
    if (index != 0) {
        index /= FRAME_SIZE;
    }
    if (index >= FRAME_MAP_SIZE) {
        printf("Index not in correct range\n");
        return;
    }

    frame_map[index] = FREE;
}

pageframe_t kalloc_cachedframe(void) {
    size_t index = 0;
    while (frame_map[index] != FREE) {
        index++;
        if (index >= FRAME_MAP_SIZE) {
            kerror("Out of memory!\n");
            return (pageframe_t) -1;
        }
    }

    frame_map[index] = USED;
    return (pageframe_t) (index*FRAME_SIZE + FRAME_START);
}


