
#include <kernel/paging.h>
#include <kernel/memory.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>

//extern void enablePaging(void);
//extern void enablePaging(pagedirectory_t);
extern void enablePaging(void);
extern void flushPaging(void);

//pagedirectory_t page_directory;

void paging_initialize(void) {

    // Sets default attributes for all page-directory entries
    /** This sets the following flags to the pages:
     *   Supervisor: Only kernel-mode can access them
     *   Write Enabled: It can be both read from and written to
     *   Not Present: The page table is not present
     */
    //page_directory = new_pagedirectory(true, true);

    // Identity-map first page
    //map_pagetable(0, 0, true, true);

    //loadPageDirectory(page_directory);
    //enablePaging();
}

pagedirectory_t new_pagedirectory(bool writable, bool kernel) {
    pagedirectory_t pd = (pagedirectory_t) kalloc_frame();

    // Not present
    unsigned int flags = (!kernel << 2) | (writable << 1);
    for (size_t i = 0; i < 1024; i++) {
        pd[i] = flags;
    }

    return pd;
}

void use_system_pagedirectory(void) {
    loadPageDirectory(page_directory);
}

pagedirectory_t copy_system_pagedirectory(void) {

    pagedirectory_t pd = (pagedirectory_t) kalloc_frame();

    for (size_t i = 0; i < 1024; i++) {
        /*
        // If system page directory's pagetable is present
        if (page_directory[i] & 1) {

            // Then allocate new

        } else {
            pd[i] = page_directory[i];
        }
        */

        pd[i] = page_directory[i];
    }

    return pd;
}


void map_pagetable(size_t physicalIndex, size_t virtualIndex, bool writable, bool kernel) {

    pagetable_t pagetable;

    // Check if page-table is present
    if (page_directory[virtualIndex] & 1) {
        pagetable = (pagetable_t) (page_directory[virtualIndex] & 0xFFFFF000);

        printf("Using existing pagetable at 0x%x\n", (unsigned int) pagetable);
    } else {
        // Allocate new pagetable if it isn't present
        pagetable = (pagetable_t) kalloc_frame();

        printf("Allocated pagetable at 0x%x\n", (unsigned int) pagetable);

        /*
        malloc(&pagetable, 0, FRAME_4KB);
        unsigned int flags = page_directory[virtualIndex] & 0x3;
        page_directory[virtualIndex] = ((unsigned int) pagetable) | flags;
        */
    }

    unsigned int flags = (!kernel << 2) | (writable << 1) | 1;

    for (size_t i = 0; i < 1024; i++) {
        // Sets address and attributes for all pages in pagetable
        //pagetable[i] = (physicalIndex * FRAME_4KB + i) | flags;
        pagetable[i] = (physicalIndex * FRAME_4MB + i * FRAME_4KB) | flags;
    }

    page_directory[virtualIndex] = ((unsigned int) pagetable) | flags;

    // TODO is it faster or slower to invlpg for all pages, or invalidate entire directory?
    flushPaging();
}

void map_pagetable_pd(pagedirectory_t pd, size_t physicalIndex, size_t virtualIndex, bool writable, bool kernel) {

    pagetable_t pagetable;

    // Check if page-table is present
    if (pd[virtualIndex] & 1) {
        pagetable = (pagetable_t) (pd[virtualIndex] & 0xFFFFF000);
        printf("Using existing pagetable at 0x%x\n", (unsigned int) pagetable);
    } else {
        // Allocate new pagetable if it isn't present
        pagetable = (pagetable_t) kalloc_frame();

        printf("Allocated pagetable at 0x%x\n", (unsigned int) pagetable);

        /*
        malloc(&pagetable, 0, FRAME_4KB);
        unsigned int flags = page_directory[virtualIndex] & 0x3;
        page_directory[virtualIndex] = ((unsigned int) pagetable) | flags;
        */
    }

    unsigned int flags = (!kernel << 2) | (writable << 1) | 1;

    for (size_t i = 0; i < 1024; i++) {
        // Sets address and attributes for all pages in pagetable
        //pagetable[i] = (physicalIndex * FRAME_4KB + i) | flags;
        pagetable[i] = (physicalIndex * FRAME_4MB + i * FRAME_4KB) | flags;
    }

    pd[virtualIndex] = ((unsigned int) pagetable) | flags;
}

void map_page(uint32_t physicalAddr, uint32_t virtualAddr, bool writable, bool kernel) {

    uint32_t physicalPTI = physicalAddr / FRAME_4MB;
    uint32_t virtualPTI = virtualAddr / FRAME_4MB;

    // Same as mod 1024 but better
    uint32_t physicalPI = (physicalAddr / FRAME_4KB) & 0x03FF;

    pagetable_t pagetable;
    bool present = page_directory[virtualPTI] & 1;

    // Check if page-table is present
    if (present) {
        pagetable = (pagetable_t) (page_directory[virtualPTI] & 0xFFFFF000);

        printf("Using existing pagetable at 0x%x\n", (unsigned int) pagetable);
    } else {
        // Allocate new pagetable if it isn't present
        pagetable = (pagetable_t) kalloc_frame();

        printf("Allocated pagetable at 0x%x\n", (unsigned int) pagetable);

        /*
        malloc(&pagetable, 0, FRAME_4KB);
        unsigned int flags = page_directory[virtualIndex] & 0x3;
        page_directory[virtualIndex] = ((unsigned int) pagetable) | flags;
        */
    }

    unsigned int flags = (!kernel << 2) | (writable << 1) | 1;

    // Sets address and attributes for all pages in pagetable
    pagetable[physicalPI] = (physicalPTI * FRAME_4MB + physicalPI * FRAME_4KB) | flags;

    // TODO should I change flags of entire pagetable or keep?
    if (!present) {
        page_directory[virtualPTI] = ((unsigned int) pagetable) | flags;
    }

    // TODO is it faster or slower to invlpg for all pages, or invalidate entire directory?
    flushPaging();
}

void map_page_pd(pagedirectory_t pd, uint32_t physicalAddr, uint32_t virtualAddr, bool writable, bool kernel) {

    uint32_t physicalPTI = physicalAddr / FRAME_4MB;
    uint32_t virtualPTI = virtualAddr / FRAME_4MB;

    // Same as mod 1024 but better
    uint32_t physicalPI = (physicalAddr / FRAME_4KB) & 0x03FF;

    pagetable_t pagetable;
    bool present = pd[virtualPTI] & 1;

    // Check if page-table is present
    if (present) {
        pagetable = (pagetable_t) (pd[virtualPTI] & 0xFFFFF000);

        printf("Using existing pagetable at 0x%x\n", (unsigned int) pagetable);
    } else {
        // Allocate new pagetable if it isn't present
        pagetable = (pagetable_t) kalloc_frame();

        printf("Allocated pagetable at 0x%x\n", (unsigned int) pagetable);

        /*
        malloc(&pagetable, 0, FRAME_4KB);
        unsigned int flags = page_directory[virtualIndex] & 0x3;
        page_directory[virtualIndex] = ((unsigned int) pagetable) | flags;
        */
    }

    unsigned int flags = (!kernel << 2) | (writable << 1) | 1;

    // Sets address and attributes for all pages in pagetable
    pagetable[physicalPI] = (physicalPTI * FRAME_4MB + physicalPI * FRAME_4KB) | flags;

    // TODO should I change flags of entire pagetable or keep?
    if (!present) {
        pd[virtualPTI] = ((unsigned int) pagetable) | flags;
    }
}

void unmap_page(void* virtualaddr) {

    uint32_t pdindex = (uint32_t) virtualaddr >> 22;
    uint32_t ptindex = (uint32_t) virtualaddr >> 12 & 0x03FF;

    pagetable_t pagetable = (pagetable_t) (page_directory[pdindex] & 0xFFFFF000);

    // Set page or entry to not present
    pagetable[ptindex] = 0x00000000;

    flushPaging();
}

void unmap_pagetable(size_t index) {

    // If the page is present then free
    if (page_directory[index] & 1) {
        kfree_frame((void*) (page_directory[index] & 0xFFFFF000));
    }

    // Set the page-table to not present
    page_directory[index] = 0x00000000;

    flushPaging();
}

