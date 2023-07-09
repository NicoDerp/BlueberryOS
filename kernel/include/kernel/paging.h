
#ifndef KERNEL_PAGING_H
#define KERNEL_PAGING_H

#include <kernel/memory.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <stdio.h>

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

#define p_to_v(n) ((n) + 0xC0000000)
#define v_to_p(n) ((n) - 0xC0000000)

#define PAGE_PRESENT   (1 << 0)
#define PAGE_READWRITE (1 << 1)
#define PAGE_USER      (1 << 2)
#define PAGE_MMAPPED   (1 << 9)

typedef uint32_t* pagetable_t;

typedef uint32_t* pagedirectory_t;

//extern char _page_directory;
//#define page_directory ((pagedirectory_t) &_page_directory)
extern uint32_t page_directory[];

extern void loadPageDirectory(pagedirectory_t);

void paging_initialize(void);
void use_system_pagedirectory(void);
pagedirectory_t new_pagedirectory(bool writable, bool kernel);
pagedirectory_t copy_system_pagedirectory(void);

pagetable_t getPagetable(uint32_t entry);
uint32_t getPageLocation(uint32_t entry);

void map_pagetable(size_t physicalIndex, size_t virtualIndex, bool writable, bool kernel);
void map_pagetable_pd(pagedirectory_t pd, size_t physicalIndex, size_t virtualIndex, bool writable, bool kernel);

void map_page(uint32_t physicalAddr, uint32_t virtualAddr, bool writable, bool kernel);
void map_page_pd(pagedirectory_t pd, uint32_t physicalAddr, uint32_t virtualAddr, bool writable, bool kernel);
void map_page_wflags_pd(pagedirectory_t pd, uint32_t physicalAddr, uint32_t virtualAddr, uint32_t flags);

void map_page_wtable_pd(pagedirectory_t pd, uint32_t physicalAddr, uint32_t virtualAddr, bool pwritable, bool pkernel, bool twritable, bool tkernel);
void set_pagetable_flags_pd(pagedirectory_t pd, size_t virtualIndex, bool writable, bool kernel);

void unmap_page(void* virtualaddr);
void unmap_pagetable(size_t index);

void printUserPagedirectory(pagedirectory_t pd);

#endif /* KERNEL_PAGING_H */

