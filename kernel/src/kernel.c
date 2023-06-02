
#include <kernel/tty.h>

#include <stdio.h>

#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/multiboot2.h>

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
void kernel_main(unsigned int test, unsigned int eax, unsigned int ebx) {

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

    printf("\nStarting BlueberryOS\n");

    printf("Setting up GDT ... ");
    gdt_initialize();
    printf("[OK]\n");

    printf("Setting up IDT ... ");
    idt_initialize();
    printf("[OK]\n");
    
    printf("\n\nWelcome to BlueberryOS!\n");

    for (size_t i = 0; i < moduleCount; i++) {
        struct multiboot_tag_module* module = modules[i];
        size_t moduleSize = module->mod_end - module->mod_start;
        printf("Module size: %d\n", moduleSize);
        printf("Contents:\n");
        for (size_t j = 0; j < moduleSize; j++) {
            printf("0x%x: 0x%x\n", j, *((unsigned char*)module->mod_start+j));
        }
        printf("Running:\n");
        module_func_t module_func = (module_func_t) module->mod_start;
        module_func();

        //printf("ooga");
        //asm volatile("int $10");

        //unsigned int num;
        //asm("\t movl %%eax,%0" : "=r" (num));

        //printf("Output is: 0x%x\n", num);
    }

    for (;;) {
        asm("hlt");
    }

}


