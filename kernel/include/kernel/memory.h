
#ifndef KERNEL_MEMORY_H
#define KERNEL_MEMORY_H

#include <stdint.h>


extern unsigned int _kernelend;
extern unsigned int _kernelstart;

#define KERNEL_END ((unsigned int) &_kernelend)
#define KERNEL_START ((unsigned int) &_kernelstart)

// TODO remove after kmalloc
#define FRAME_MAP_SIZE 90

#define FRAME_1KB ((unsigned int) 0x400)
#define FRAME_4KB ((unsigned int) 0x1000)

#define FRAME_1MB ((unsigned int) 0x100000)
#define FRAME_4MB ((unsigned int) 0x400000)

#define FRAME_SIZE FRAME_4KB
//#define MAX_FRAMES (FRAME_SIZE / FRAME_MAP_SIZE)
#define FRAME_START (FRAME_SIZE - (KERNEL_END) % FRAME_SIZE + KERNEL_END)
//#define FRAME_START (FRAME_SIZE - (kernelend - 1) % FRAME_SIZE + kernelend-1)
//#define FRAME_START (FRAME_SIZE - (KERNEL_END) % FRAME_SIZE + KERNEL_END)
#define FRAME_CACHE_SIZE 8

typedef enum {
    FREE,
    USED
} frame_map_t;

typedef void* pageframe_t;

void memory_initialize(uint32_t framestart_, uint32_t bytes);

pageframe_t kalloc_frame(void);
pageframe_t kalloc_frames(unsigned int count);
void kfree_frame(pageframe_t frame);

uint32_t get_used_memory(void);

#endif /* KERNEL_MEMORY_H */

