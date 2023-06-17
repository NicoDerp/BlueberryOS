
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

#define VERBOSE 0

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

    //                              0xC0000000
    //uint16_t* location = (uint16_t*) (0xC00B8000 + 0);
    //uint16_t* location = (uint16_t*) (0xC03FF000 + 0);
    //*location = 1;


    /* Initialize framebuffer first-thing */
    /* Copied from https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html#kernel_002ec */
    /*
    for (tag = (struct multiboot_tag*) (ebx + 8);
       tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (struct multiboot_tag*) ((multiboot_uint8_t*) tag + ((tag->size + 7) & ~7)))
    {
        if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER)
        {
            struct multiboot_tag_framebuffer *tagfb = (struct multiboot_tag_framebuffer*) tag;
*/
            /* Initialize framebuffer */
/*
            terminal_initialize((size_t) tagfb->common.framebuffer_width, (size_t) tagfb->common.framebuffer_height, (void*) (unsigned long) tagfb->common.framebuffer_addr);
            break;
        }
    }
    */

    terminal_initialize(80, 25, (void*) 0xC03FF000);

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

    printf("Setting up Kernel Stack ...");
    uint32_t esp;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    set_kernel_stack(esp);
    printf("[OK]\n");


    if (eax != 0x36d76289) {
        printf("[ERROR] Failed to verify if bootloader has passed correct information\n");
    }



    struct multiboot_tag_module modules[32];
    size_t moduleCount = 0;

#if VERBOSE == 1
    printf("kernelstart: 0x%x\n", KERNEL_START);
    printf("kernelend: 0x%x\n", KERNEL_END);
#endif

    struct multiboot_tag* tag;
    for (tag = (struct multiboot_tag*) (ebx + 8);
       tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (struct multiboot_tag*) ((multiboot_uint8_t*) tag + ((tag->size + 7) & ~7)))
    {
#if VERBOSE == 1
        printf("Tag 0x%x, Size 0x%x\n", tag->type, tag->size);
#endif

        switch (tag->type)
        {
            case MULTIBOOT_TAG_TYPE_CMDLINE:
#if VERBOSE == 1
                printf("Command line = '%s'\n", ((struct multiboot_tag_string *) tag)->string);
#endif
                break;
            case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
#if VERBOSE == 1
                printf("Boot loader name = '%s'\n", ((struct multiboot_tag_string *) tag)->string);
#endif
                break;
            case MULTIBOOT_TAG_TYPE_MODULE:
                struct multiboot_tag_module* tag_module = (struct multiboot_tag_module*) tag;

#if VERBOSE == 1
                printf("Module at 0x%x-0x%x. Command line '%s'\n",
                    tag_module->mod_start + 0xC0000000,
                    tag_module->mod_end + 0xC0000000,
                    tag_module->cmdline + 0xC0000000);
#endif
                size_t mod_size = tag_module->mod_end - tag_module->mod_start;

                memcpy(&modules[moduleCount++], tag_module, mod_size);
                break;
            case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
#if VERBOSE == 1
                printf("mem_lower = %uKB, mem_upper = %uKB\n",
                  ((struct multiboot_tag_basic_meminfo *) tag)->mem_lower,
                  ((struct multiboot_tag_basic_meminfo *) tag)->mem_upper);
#endif
                break;
            case MULTIBOOT_TAG_TYPE_BOOTDEV:
#if VERBOSE == 1
                printf("Boot device 0x%x,%u,%u\n",
                  ((struct multiboot_tag_bootdev *) tag)->biosdev,
                  ((struct multiboot_tag_bootdev *) tag)->slice,
                  ((struct multiboot_tag_bootdev *) tag)->part);
#endif
                break;
            case MULTIBOOT_TAG_TYPE_MMAP:
                {
#if VERBOSE == 1
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
#endif
                }
                break;
        }
    }

    printf("\n\nWelcome to BlueberryOS!\n");

    printf("Modules: %d\n", moduleCount);

    //syscall(SYS_write, STDOUT_FILENO, "hello\n", 6);

    // 0xC03FF000

    for (size_t i = 0; i < moduleCount; i++) {

        struct multiboot_tag_module module = modules[i];
        module.mod_start = p_to_v(module.mod_start);
        module.mod_end = p_to_v(module.mod_end);

        size_t moduleSize = module.mod_end - module.mod_start;
        printf("Module size: %d\n", moduleSize);

        process_t* process = newProcess("Ooga booga", &module);
        (void) process;

        printf("Process info:\n");
        printf(" - Name: '%s'\n", process->name);
        printf(" - Id: %d\n", process->id);
        printf(" - Entry point: 0x%x\n", process->entryPoint);
        printf(" - Pagetables:\n");

        for (size_t j = 0; j < 767; j++) {
            if (process->pd[j] & 1) {
                printf("   - %d: 0x%x\n", j, process->pd[j]);
                pagetable_t pagetable = (pagetable_t) p_to_v(process->pd[j] & 0xFFFFF000);
                for (size_t k = 0; k < 1024; k++) {
                    if (pagetable[k] & 1) {
                        printf("     - Page %d: 0x%x\n", k, pagetable[k]);
                    }
                }
            }
        }

        loadPageDirectory(process->pd);

        //printf("test\n");

        /*
        char* byte = (char*) (0x08049098);
        for (size_t j = 0; j < 10; j++) {
            printf("byte %d: 0x%x, '%c'\n", j, ((unsigned int) byte[j]) & 0xFF, byte[j]);
        }
        */

        /*
        pagetable_t pagetable = (pagetable_t) ((process->pd[0] & 0xFFFFF000) + 0xC0000000);
        printf("p: 0x%x\n", (unsigned int) pagetable);
        for (size_t j = 0; j < 8; j++) {
            printf("%d: 0x%x\n", j, pagetable[j]);
        }
        */

        //loadPageDirectory(page_directory);

        //pagedirectory_t pd = (pagedirectory_t) &((char) page_directory);

        //runProcess(process);
    }

    if (moduleCount == 0) {
        printf("No modules!\n");
        return;
    }


    for (;;) {};

    /*
    int a = syscall(SYS_write, STDOUT_FILENO, "Hello world!\n", 13);
    printf("Out: %d\n", a);
    */

    for (;;) {
        asm("hlt");
    }

}


