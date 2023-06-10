
#include <kernel/tty.h>

#include <kernel/multiboot2.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/usermode.h>
#include <kernel/paging.h>
#include <kernel/memory.h>
#include <kernel/file.h>

#include <string.h>
#include <stdio.h>

#include <sys/syscall.h>
#include <unistd.h>

/* Check if you are targeting the wrong operating system */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certaintly run into trouble."
#endif


/* Check if the compiler isn't made for 32-bit ix86-elf targets */
#if !defined(__i386__)
#error "The kernel needs to be compiled with an ix86-elf compiler"
#endif

typedef void (*module_func_t)(void);

//void kernel_main(const multiboot_header* mutliboot_structure) {
void kernel_main(unsigned int eax, unsigned int ebx) {

    /*
    for (int y = 0; y < 100; y++) {
        for (int x = 0; x < 100; x+=3) {
            unsigned char* location = (unsigned char*) 0xA0000 + 768 * y + x;
            *location = 2;
        }
    }*/

    if (eax != 0x36d76289) {
        printf("[ERROR] Failed to verify if bootloader has passed correct information");
    }

    /* Initialize framebuffer first-thing */
    /* Copied from https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html#kernel_002ec */
    struct multiboot_tag* tag;
    for (tag = (struct multiboot_tag*) (ebx + 8);
       tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (struct multiboot_tag*) ((multiboot_uint8_t*) tag + ((tag->size + 7) & ~7)))
    {
        if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER)
        {
            struct multiboot_tag_framebuffer *tagfb = (struct multiboot_tag_framebuffer*) tag;

            /* Initialize framebuffer */
            terminal_initialize((size_t) tagfb->common.framebuffer_width, (size_t) tagfb->common.framebuffer_height, (void*) (unsigned long) tagfb->common.framebuffer_addr);
            break;
        }
    }

    printf("start\n");

    struct multiboot_tag_module* modules[32];
    size_t moduleCount = 0;

    for (tag = (struct multiboot_tag*) (ebx + 8);
       tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (struct multiboot_tag*) ((multiboot_uint8_t*) tag + ((tag->size + 7) & ~7)))
    {
        printf("Tag 0x%x, Size 0x%x\n", tag->type, tag->size);
        switch (tag->type)
        {
            case MULTIBOOT_TAG_TYPE_CMDLINE:
                printf("Command line = '%s'\n", ((struct multiboot_tag_string *) tag)->string);
                break;
            case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
                printf("Boot loader name = '%s'\n", ((struct multiboot_tag_string *) tag)->string);
                break;
            case MULTIBOOT_TAG_TYPE_MODULE:
                printf("Module at 0x%x-0x%x. Command line '%s'\n",
                ((struct multiboot_tag_module *) tag)->mod_start,
                ((struct multiboot_tag_module *) tag)->mod_end,
                ((struct multiboot_tag_module *) tag)->cmdline);

                modules[moduleCount++] = (struct multiboot_tag_module*) tag;
                break;
            case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
                printf("mem_lower = %uKB, mem_upper = %uKB\n",
                  ((struct multiboot_tag_basic_meminfo *) tag)->mem_lower,
                  ((struct multiboot_tag_basic_meminfo *) tag)->mem_upper);
                break;
            case MULTIBOOT_TAG_TYPE_BOOTDEV:
                printf("Boot device 0x%x,%u,%u\n",
                  ((struct multiboot_tag_bootdev *) tag)->biosdev,
                  ((struct multiboot_tag_bootdev *) tag)->slice,
                  ((struct multiboot_tag_bootdev *) tag)->part);
                break;
            case MULTIBOOT_TAG_TYPE_MMAP:
                {
                    multiboot_memory_map_t *mmap;

                    printf("mmap\n");

                    for (mmap = ((struct multiboot_tag_mmap *) tag)->entries;
                        (multiboot_uint8_t*) mmap < (multiboot_uint8_t *) tag + tag->size;
                        mmap = (multiboot_memory_map_t *) ((unsigned long) mmap + ((struct multiboot_tag_mmap *) tag)->entry_size)) {

                        printf(" base_addr = 0x%x%x,"
                        " length = 0x%x%x, type = 0x%x\n",
                        (unsigned) (mmap->addr >> 32),
                        (unsigned) (mmap->addr & 0xffffffff),
                        (unsigned) (mmap->len >> 32),
                        (unsigned) (mmap->len & 0xffffffff),
                        (unsigned) mmap->type);
                    }
                }
                break;
        }
    }

    extern pagedirectory_t page_directory;
    printf("\nStarting BlueberryOS\n");

    printf("Settings up System TSS ...");
    tss_initialize();
    printf("[OK]\n");

    printf("Setting up GDT ... ");
    gdt_initialize();
    printf("[OK]\n");

    printf("Flushing TSS ...");
    flush_tss();
    printf("[OK]\n");

    printf("Setting up IDT ... ");
    idt_initialize();
    printf("[OK]\n");

    printf("Setting up Paging ...");
    paging_initialize();
    printf("[OK]\n");

    /*
    change_pagetable(0, false, false);

    unsigned int* y = (unsigned int*) 0x0FFFFF;
    *y = 5;
    printf("a: 0x%x, 0x%x\n", y, *y);
    */

    printf("\n\nWelcome to BlueberryOS!\n");

    printf("Modules: %d\n", moduleCount);

    printf("Before: 0x%x\n", page_directory[0]);
    printf("Before: 0x%x\n", page_directory[1]);
    printf("Before: 0x%x\n", page_directory[2]);

    // For test, identity map
    map_pagetable(1, 1, true, false);

    printf("Before: 0x%x\n", page_directory[0]);
    printf("Before: 0x%x\n", page_directory[1]);
    printf("Before: 0x%x\n", page_directory[2]);

    for (size_t i = 0; i < moduleCount; i++) {
        struct multiboot_tag_module* module = modules[i];
        size_t moduleSize = module->mod_end - module->mod_start;
        printf("Module size: %d\n", moduleSize);

        /*
        file_t* file = loadFileFromMultiboot(module);
        (void)file;
        */

        //memcpy((void*) 0x1024, (void*) module->mod_start, moduleSize);
        memcpy((void*) FRAME_4MB, (void*) module->mod_start, moduleSize);

        /*
        printf("Contents:\n");
        for (size_t j = 0; j < moduleSize-16; j++) {
            printf("0x%x: 0x%x\n", j, *((uint8_t*) (0x1024+j)));
        }
        */

        /*
        module_func_t module_func = (module_func_t) FRAME_4MB;
        printf("Running:\n");
        module_func();
        */
    }

    if (moduleCount == 0) {
        printf("No modules!\n");
        return;
    }

    /*
    extern pagedirectory_t page_directory;
    printf("Before: 0x%x\n", page_directory[1]);
    printf("a: 0x%x\n", *((uint8_t*) 0x1000));

    map_pagetable(0, 1, true, true);

    printf("Before: 0x%x\n", page_directory[1]);
    printf("a: 0x%x\n", *((uint8_t*) 0x1000));

    unmap_pagetable(1);

    printf("After: 0x%x\n", page_directory[1]);
    */

    //syscall(SYS_write, STDOUT_FILENO, "hello", 5);

    //change_pagetable_vaddr(0, true, false);

    //printf("a: 0x%x\n", *((uint8_t*) 0x5000));

    uint32_t esp;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    set_kernel_stack(esp);

    //map_pagetable(0x5, 0x5, true, false);
    /*
    unmap_pagetable(0x4);
    unmap_pagetable(0x5);
    unmap_pagetable(0x6);
    */

    /*
    printf("Value: 0x%x\n", *((uint8_t*) (FRAME_4MB+0)));
    printf("Value: 0x%x\n", *((uint8_t*) (FRAME_4MB+1)));
    */

    // For test user-process, map virtual addr 0 to physical addr 4MB
    //map_pagetable(FRAME_4MB/FRAME_4KB, 0, true, false);

    printf("Before: 0x%x\n", page_directory[0]);
    printf("Before: 0x%x\n", page_directory[1]);
    printf("Before: 0x%x\n", page_directory[2]);

    uint32_t stack_ptr = (uint32_t) kalloc_frame();
    uint32_t virtual_stack_ptr = FRAME_4MB + 0x1000;
    uint32_t virtual_stack_top = virtual_stack_ptr + 0xF00;

    map_page(stack_ptr, virtual_stack_ptr, true, false);
    printf("User process info:\n");
    printf(" - Code starting at 0x%x\n", FRAME_4MB);
    printf(" - Physical stack at 0x%x\n", stack_ptr);
    printf(" - Virtual stack at 0x%x\n", virtual_stack_ptr);
    printf(" - Virtual stack-top at 0x%x\n", virtual_stack_top);

    // test
    map_page(FRAME_4MB, 0x0, true, false);

    enter_usermode(FRAME_4MB, virtual_stack_top);

    /*
    int a = syscall(SYS_write, STDOUT_FILENO, "Hello world!\n", 13);
    printf("Out: %d\n", a);
    */

    /*
    pageframe_t frame = kalloc_frame();
    printf("frame: 0x%x\n", frame);
    kfree_frame(frame);
    */


    for (;;) {
        asm("hlt");
    }

}


