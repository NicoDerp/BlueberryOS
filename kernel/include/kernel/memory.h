
#ifndef KERNEL_MEMORY_H
#define KERNEL_MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <bits/memory.h>


extern unsigned int _kernelend;
extern unsigned int _kernelstart;

#define KERNEL_END ((unsigned int) &_kernelend)
#define KERNEL_START ((unsigned int) &_kernelstart)

#define FRAME_1KB ((unsigned int) 0x400)
#define FRAME_4KB ((unsigned int) 0x1000)

#define FRAME_1MB ((unsigned int) 0x100000)
#define FRAME_4MB ((unsigned int) 0x400000)

#define FRAME_SIZE FRAME_4KB
//#define MAX_FRAMES (FRAME_SIZE / FRAME_MAP_SIZE)
#define FRAME_START (FRAME_SIZE - (KERNEL_END) % FRAME_SIZE + KERNEL_END)

#define MAX_FRAME_CACHE_SIZE 32

typedef void* pageframe_t;
void memory_initialize(uint32_t total, uint32_t frames);
void memory_mark_allocated(uint32_t start, uint32_t end);

void* kmalloc(size_t size);
void kfree(void* ptr);
void* krealloc(void* ptr, size_t size);
void* kcalloc(size_t items, size_t count);

pageframe_t kalloc_frame(void);
pageframe_t kalloc_frames(unsigned int count);
void kfree_frame(pageframe_t frame);
void kfree_frames(pageframe_t frame, unsigned int count);

uint32_t get_used_memory(void);

#endif /* KERNEL_MEMORY_H */

