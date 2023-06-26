
#ifndef KERNEL_MEMORY_H
#define KERNEL_MEMORY_H

extern unsigned int _kernelend;
extern unsigned int _kernelstart;

#define KERNEL_END ((unsigned int) &_kernelend)
#define KERNEL_START ((unsigned int) &_kernelstart)

#define FRAME_MAP_SIZE 64

#define FRAME_4KB ((unsigned int) 0x1000)
#define FRAME_4MB ((unsigned int) 0x400000)

#define FRAME_SIZE FRAME_4KB
#define MAX_FRAMES (FRAME_SIZE / FRAME_MAP_SIZE)
//#define FRAME_START (FRAME_SIZE - (kernelend - 1) % FRAME_SIZE + kernelend-1)
#define FRAME_START (FRAME_SIZE - (KERNEL_END) % FRAME_SIZE + KERNEL_END)
#define FRAME_CACHE_SIZE 8

typedef enum {
    FREE,
    USED
} frame_map_t;

typedef void* pageframe_t;

pageframe_t kalloc_frame(void);
void kfree_frame(pageframe_t frame);

#endif /* KERNEL_MEMORY_H */

