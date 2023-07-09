
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

// TODO remove after kmalloc
//#define FRAME_MAP_SIZE (20*8)

#define FRAME_1KB ((unsigned int) 0x400)
#define FRAME_4KB ((unsigned int) 0x1000)

#define FRAME_1MB ((unsigned int) 0x100000)
#define FRAME_4MB ((unsigned int) 0x400000)

#define FRAME_SIZE FRAME_4KB
//#define MAX_FRAMES (FRAME_SIZE / FRAME_MAP_SIZE)
#define FRAME_START (FRAME_SIZE - (KERNEL_END) % FRAME_SIZE + KERNEL_END)
#define FRAME_CACHE_SIZE 8

#define MEMORY_TAG_MAGIC    0xFEAB0CBD
#define MEMORY_MAX_COMPLETE 4
#define MEMORY_MAX_EXP      32
#define MEMORY_MIN_EXP      4
#define MEMORY_TOT_EXP      (MEMORY_MAX_EXP - MEMORY_MIN_EXP)
#define MEMORY_MIN_FRAMES   8

typedef void* pageframe_t;
void memory_initialize(uint32_t framestart_, uint32_t bytes);

void* kmalloc(size_t);
void kfree(void*);

pageframe_t kalloc_frame(void);
pageframe_t kalloc_frames(unsigned int count);
void kfree_frame(pageframe_t frame);
void kfree_frames(pageframe_t frame, unsigned int count);

uint32_t get_used_memory(void);

#endif /* KERNEL_MEMORY_H */

