
#ifndef KERNEL_PAGING_H
#define KERNEL_PAGING_H

#include <kernel/memory.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*
typedef struct {
    uint32_t present:1;
    uint32_t readwrite:1;
    uint32_t usermode:1;
    uint32_t reserved:2;
    uint32_t available:3;
    uint32_t address:20;
} page_t;

typedef struct {
    uint32_t present:1;
    uint32_t readwrite:1;
    uint32_t usermode:1;
    uint32_t reserved:2;
    uint32_t available:3;
    uint32_t address:20;
} pagetable_t;
*/

typedef uint32_t* pagetable_t;

typedef uint32_t* pagedirectory_t;

extern void loadPageDirectory(pagedirectory_t);

void paging_initialize(void);
pagedirectory_t new_pagedirectory(bool writable, bool kernel);
void map_pagetable(size_t physicalIndex, size_t virtualIndex, bool writable, bool kernel);
void map_page(uint32_t physicalAddr, uint32_t virtualAddr, bool writable, bool kernel);
void map_page_pd(pagedirectory_t pd, uint32_t physicalAddr, uint32_t virtualAddr, bool writable, bool kernel);
void unmap_page(void* virtualaddr);
void unmap_pagetable(size_t index);

#endif /* KERNEL_PAGING_H */

