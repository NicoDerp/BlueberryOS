
#ifndef KERNEL_PAGING_H
#define KERNEL_PAGING_H

#include <kernel/memory.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    unsigned char present:1;
    unsigned char readwrite:1;
    unsigned char usermode:1;
    unsigned char reserved:2;
    unsigned char available:3;
    unsigned int address:20;
} page_t;

typedef uint32_t* pagetable_t;

typedef uint32_t* pagedirectory_t;

void paging_initialize(void);
void change_pagetable(size_t index, bool writable, bool kernel);

#endif /* KERNEL_PAGING_H */

