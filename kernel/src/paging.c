
#include <kernel/paging.h>
#include <kernel/memory.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>

//extern void enablePaging(void);
//extern void loadPageDirectory(unsigned int*);
extern void enablePaging(pagedirectory_t);
extern void flushPaging(void);

pagedirectory_t page_directory;

void paging_initialize(void) {
    page_directory = (pagedirectory_t) kalloc_frame();
    //first_page_table = (pagetable_t) kalloc_frame();

    for (size_t i = 0; i < 1024; i++) {
        /** This sets the following flags to the pages:
         *   Supervisor: Only kernel-mode can access them
         *   Write Enabled: It can be both read from and written to
         *   Not Present: The page table is not present
         */
        page_directory[i] = 0x00000002;
    }

    /*
    for (size_t i = 0; i < 1024; i++) {
        // attributes: supervisor level, read/write, present.
        first_page_table[i] = (i * FRAME_4KB) | 3;
    }
    page_directory[0] = ((unsigned int) first_page_table) | 3;
    */

    // Map first page
    change_pagetable(0, true, true);

    //loadPageDirectory(page_directory);
    //enablePaging();
    enablePaging(page_directory);
}

void change_pagetable(size_t index, bool writable, bool kernel) {

    pagetable_t pagetable;

    // Check if page-table is present
    if (page_directory[index] & 1) {
        pagetable = (pagetable_t) (page_directory[index] & 0xFFFFF000);
    } else {
        // Allocate new pagetable if it isn't present
        pagetable = (pagetable_t) kalloc_frame();
        unsigned int flags = page_directory[index] & 0x3;
        page_directory[index] = ((unsigned int) pagetable) | flags;
    }

    unsigned int flags = (!kernel << 2) | (writable << 1) | 1;

    for (size_t i = 0; i < 1024; i++) {
        // sets attrbitues
        pagetable[i] = (i * FRAME_4KB) | flags;
    }

    page_directory[index] = ((unsigned int) pagetable) | flags;

    flushPaging();
}

void unmap_page(void* virtualaddr) {

    uint32_t pdindex = (uint32_t) virtualaddr >> 22;
    uint32_t ptindex = (uint32_t) virtualaddr >> 12 & 0x03FF;

    pagetable_t pagetable = (pagetable_t) (page_directory[pdindex] & 0xFFFFF000);

    // Set page or entry to not present
    pagetable[ptindex] = 0x00000000;

    flushPaging();
}

void unmap_pagetable(void* virtualaddr) {

    uint32_t pdindex = (uint32_t) virtualaddr >> 22;

    // If the page is present then free
    if (page_directory[pdindex] & 1) {
        kfree_frame((void*) (page_directory[pdindex] & 0xFFFFF000));
    }

    // Set the page-table to not present
    page_directory[pdindex] = 0x00000000;

    flushPaging();
}

