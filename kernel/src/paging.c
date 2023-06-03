
#include <kernel/paging.h>
#include <kernel/memory.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>

extern void enablePaging(void);
extern void loadPageDirectory(unsigned int*);

pagedirectory_t page_directory;
pagetable_t* pagetables[1024];

void paging_initialize(void) {
    page_directory = (pagedirectory_t) kalloc_frame();
    //first_page_table = (pagetable_t) kalloc_frame();

    for (size_t i = 0; i < 1024; i++) {
        pagetables[i] = 0;

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

    change_pagetable(0, true, true);

    loadPageDirectory(page_directory);
    enablePaging();
}

void change_pagetable(size_t index, bool writable, bool kernel) {

    pagetable_t pagetable;

    if (pagetables[index] == 0) {
        pagetable = (pagetable_t) kalloc_frame();
        pagetables[index] = &pagetable;
    } else {
        pagetable = *pagetables[index];
    }

    unsigned int flags = (!kernel << 2) | (writable << 1) | 1;

    for (size_t i = 0; i < 1024; i++) {
        // sets attrbitues
        pagetable[i] = (i * FRAME_4KB) | flags;
    }

    page_directory[index] = ((unsigned int) pagetable) | flags;
}

