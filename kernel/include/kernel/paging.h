
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

void paging_initialize(void);
void change_pagetable(size_t index, bool writable, bool kernel);

#endif /* KERNEL_PAGING_H */

