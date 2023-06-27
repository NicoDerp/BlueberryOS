
#include <kernel/paging.h>
#include <kernel/memory.h>
#include <kernel/logging.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

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

        // The first won't be present anyways
        pd[i] = page_directory[i];
    }

    return pd;
}

void freeUserPagedirectory(pagedirectory_t pd) {

    for (size_t i = 0; i < 768; i++) {
        if (pd[i] & 1) {
            kfree_frame((void*) p_to_v(pd[i] & 0xFFFFF000));
        }
    }

    kfree_frame(pd);
}

void map_pagetable(size_t physicalIndex, size_t virtualIndex, bool writable, bool kernel) {

    map_pagetable_pd(page_directory, physicalIndex, virtualIndex, writable, kernel);

    // TODO is it faster or slower to invlpg for all pages, or invalidate entire directory?
    flushPaging();
}

void map_pagetable_pd(pagedirectory_t pd, size_t physicalIndex, size_t virtualIndex, bool writable, bool kernel) {

    pagetable_t pagetable;

    // Check if page-table is present
    if (pd[virtualIndex] & 1) {
        pagetable = (pagetable_t) p_to_v(pd[virtualIndex] & 0xFFFFF000);

        VERBOSE("map_pagetable_pd: Using existing pagetable at 0x%x\n", (unsigned int) pagetable);
    } else {
        // Allocate new pagetable if it isn't present
        pagetable = (pagetable_t) kalloc_frame();

        VERBOSE("map_pagetable_pd: Allocated pagetable at 0x%x\n", (unsigned int) pagetable);

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

    pd[virtualIndex] = v_to_p((unsigned int) pagetable) | flags;
}

void map_page_wtable_pd(pagedirectory_t pd, uint32_t physicalAddr, uint32_t virtualAddr, bool pwritable, bool pkernel, bool twritable, bool tkernel) {

    uint32_t physicalPTI = physicalAddr / FRAME_4MB;
    uint32_t virtualPTI = virtualAddr / FRAME_4MB;

    // Same as mod 1024 but better
    uint32_t physicalPI = (physicalAddr / FRAME_4KB) & 0x03FF;
    uint32_t virtualPI = (virtualAddr / FRAME_4KB) & 0x03FF;

    pagetable_t pagetable;
    bool present = pd[virtualPTI] & 1;

    // Check if page-table is present
    if (present) {
        pagetable = (pagetable_t) p_to_v(pd[virtualPTI] & 0xFFFFF000);

        VERBOSE("map_page_wtable_pd: Using existing pagetable at 0x%x\n", (unsigned int) pagetable);
    } else {
        // Allocate new pagetable if it isn't present
        pagetable = (pagetable_t) kalloc_frame();

        VERBOSE("map_page_wtable_pd: Allocated pagetable at 0x%x\n", (unsigned int) pagetable);

        memset(pagetable, 0, FRAME_4KB);
    }

    unsigned int pflags = (!pkernel << 2) | (pwritable << 1) | 1;
    unsigned int tflags = (!tkernel << 2) | (twritable << 1) | 1;

    VERBOSE("map_page_wtable_pd: Mapping 0x%x(p) to 0x%x(v). rw%d, k%d\n", physicalPTI*FRAME_4MB+physicalPI*FRAME_4KB, virtualPTI*FRAME_4MB+virtualPI*FRAME_4KB, pwritable, pkernel);
    VERBOSE("map_page_wtable_pd: Setting pagetable flags rw%d and k%d\n", twritable, tkernel);

    // Sets address and attributes for page
    pagetable[virtualPI] = (physicalPI * FRAME_4KB + physicalPTI * FRAME_4MB) | pflags;

    // Update pagetable and also set pagetable flags
    pd[virtualPTI] = v_to_p((unsigned int) pagetable) | tflags;
}

void set_pagetable_flags_pd(pagedirectory_t pd, size_t virtualIndex, bool writable, bool kernel) {

    pagetable_t pagetable;

    // Check if page-table is present
    if (pd[virtualIndex] & 1) {
        pagetable = (pagetable_t) p_to_v(pd[virtualIndex] & 0xFFFFF000);

        VERBOSE("set_pagetable_flags_pd: Using existing pagetable at 0x%x\n", (unsigned int) pagetable);
    } else {
        // Allocate new pagetable if it isn't present
        pagetable = (pagetable_t) kalloc_frame();

        VERBOSE("[warning] set_pagetable_flags_pd: Allocated pagetable at 0x%x\n", (unsigned int) pagetable);

        /*
        malloc(&pagetable, 0, FRAME_4KB);
        unsigned int flags = page_directory[virtualIndex] & 0x3;
        page_directory[virtualIndex] = ((unsigned int) pagetable) | flags;
        */
    }

    unsigned int flags = (!kernel << 2) | (writable << 1) | 1;

    pd[virtualIndex] = v_to_p((unsigned int) pagetable) | flags;
}

void map_page(uint32_t physicalAddr, uint32_t virtualAddr, bool writable, bool kernel) {

    map_page_pd(page_directory, physicalAddr, virtualAddr, writable, kernel);

    // TODO is it faster or slower to invlpg for all pages, or invalidate entire directory?
    flushPaging();
}

void map_page_pd(pagedirectory_t pd, uint32_t physicalAddr, uint32_t virtualAddr, bool writable, bool kernel) {

    uint32_t physicalPTI = physicalAddr / FRAME_4MB;
    uint32_t virtualPTI = virtualAddr / FRAME_4MB;

    // Same as mod 1024 but better
    uint32_t physicalPI = (physicalAddr / FRAME_4KB) & 0x03FF;
    uint32_t virtualPI = (virtualAddr / FRAME_4KB) & 0x03FF;

    pagetable_t pagetable;
    bool present = pd[virtualPTI] & 1;

    // Check if page-table is present
    if (present) {
        pagetable = (pagetable_t) p_to_v(pd[virtualPTI] & 0xFFFFF000);

        VERBOSE("map_page_pd: Using existing pagetable at 0x%x\n", (unsigned int) pagetable);
    } else {
        // Allocate new pagetable if it isn't present
        pagetable = (pagetable_t) kalloc_frame();

        VERBOSE("map_page_pd: Allocated pagetable at 0x%x\n", (unsigned int) pagetable);

        memset(pagetable, 0, FRAME_4KB);
    }

    unsigned int flags = (!kernel << 2) | (writable << 1) | 1;

    VERBOSE("map_page_pd: Mapping 0x%x(p) to 0x%x(v). rw%d, k%d\n", physicalPTI*FRAME_4MB+physicalPI*FRAME_4KB, virtualPTI*FRAME_4MB+virtualPI*FRAME_4KB, writable, kernel);

    // Sets address and attributes for page
    pagetable[virtualPI] = (physicalPI * FRAME_4KB + physicalPTI * FRAME_4MB) | flags;

    // TODO should I change flags of entire pagetable or keep?
    if (!present) {
        pd[virtualPTI] = v_to_p((unsigned int) pagetable) | flags;
    }
}

void unmap_page(void* virtualaddr) {

    uint32_t pdindex = (uint32_t) virtualaddr >> 22;
    uint32_t ptindex = (uint32_t) virtualaddr >> 12 & 0x03FF;

    pagetable_t pagetable = (pagetable_t) p_to_v(page_directory[pdindex] & 0xFFFFF000);

    // Set page or entry to not present
    pagetable[ptindex] = 0x00000000;

    flushPaging();
}

void unmap_pagetable(size_t index) {

    // If the page is present then free
    if (page_directory[index] & 1) {
        kfree_frame((void*) p_to_v(page_directory[index] & 0xFFFFF000));
    }

    // Set the page-table to not present
    page_directory[index] = 0x00000000;

    flushPaging();
}



void printUserPagedirectory(pagedirectory_t pd) {

    for (size_t j = 0; j < 768; j++) {
        if (pd[j] & 1) {
            printf("   - %d: 0x%x\n", j, pd[j]);
            pagetable_t pagetable = (pagetable_t) p_to_v(pd[j] & 0xFFFFF000);
            for (size_t k = 0; k < 1024; k++) {
                if (pagetable[k] & 1) {
                    bool rw = pagetable[k] & 0x2;
                    bool kernel = !(pagetable[k] & 0x4);
                    printf("     - Page %d for 0x%x rw %d k %d: 0x%x\n", k, FRAME_4KB*k + FRAME_4MB*j, rw, kernel, pagetable[k] & ~(0x7));
                }
            }
        }
    }
}



