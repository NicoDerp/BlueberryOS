
#include <kernel/paging.h>
#include <kernel/memory.h>
#include <stddef.h>

unsigned int* page_directory;

void paging_initialize(void) {
    page_directory = (unsigned int*) kalloc_frame();

    for (size_t i = 0; i < 1024; i++) {
        /** This sets the following flags to the pages:
         *   Supervisor: Only kernel-mode can access them
         *   Write Enabled: It can be both read from and written to
         *   Not Present: The page table is not present
         */
        page_directory[i] = 0x00000002;
    }
}

