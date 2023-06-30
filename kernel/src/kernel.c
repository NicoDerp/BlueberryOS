
#include <kernel/logging.h>
#include <kernel/errors.h>

#include <kernel/tty.h>
#include <kernel/multiboot2.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/usermode.h>
#include <kernel/paging.h>
#include <kernel/memory.h>
#include <kernel/file.h>
#include <kernel/io.h>

#include <tinf/tinf.h>

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






char* titleText = "\e[b;0m"
"  ____  _            _                           ____   _____ \n"
" |  _ \\| |          | |                         / __ \\ / ____|\n"
" | |_) | |_   _  ___| |__   ___ _ __ _ __ _   _| |  | | (___  \n"
" |  _ <| | | | |/ _ \\ '_ \\ / _ \\ '__| '__| | | | |  | |\\___ \\\n"
" | |_) | | |_| |  __/ |_) |  __/ |  | |  | |_| | |__| |____) |\n"
" |____/|_|\\__,_|\\___|_.__/ \\___|_|  |_|   \\__, |\\____/|_____/\n"
"                                           __/ |              \n"
"                                          |___/               \n\e[0m";





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

    uint32_t memorySize;
    uint32_t virtualFramebuffer;
    bool foundMemory = false;

    struct multiboot_tag* tag;
    for (tag = (struct multiboot_tag*) (ebx + 8);
       tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (struct multiboot_tag*) ((multiboot_uint8_t*) tag + ((tag->size + 7) & ~7)))
    {
        switch (tag->type)
        {
            case MULTIBOOT_TAG_TYPE_MMAP:
                {
                    multiboot_memory_map_t *mmap;

                    for (mmap = ((struct multiboot_tag_mmap *) tag)->entries;
                        (multiboot_uint8_t*) mmap < (multiboot_uint8_t *) tag + tag->size;
                        mmap = (multiboot_memory_map_t *) ((unsigned long) mmap + ((struct multiboot_tag_mmap *) tag)->entry_size)) {

                        unsigned int addr = p_to_v((unsigned int) (mmap->addr & 0xFFFFFFFF));
                        unsigned int len = (unsigned int) (mmap->len & 0xFFFFFFFF);

                        // Minimum requirement
                        if ((mmap->type == MULTIBOOT_MEMORY_AVAILABLE) && (addr + len > FRAME_START + 2*FRAME_4MB)) {
                            foundMemory = true;
                            virtualFramebuffer = addr + len - FRAME_4KB;
                            memorySize = len - FRAME_4KB;
                        }
                    }
                }
                break;
        }
    }

    if (!foundMemory) {

        // Enough for just one pagetable
        memory_initialize(FRAME_START, FRAME_4KB);
        virtualFramebuffer = 0xC03FF000;
        map_page(0x000B8000, virtualFramebuffer, true, true);
        terminal_initialize(80, 25, (void*) virtualFramebuffer);

        printf("\n\n[\e[4;0m[FATAL]\e[0m Not enough memory available to run OS!\n");
        kabort();
    }

    memory_initialize(FRAME_START, memorySize);
    map_page(0x000B8000, virtualFramebuffer, true, true);
    terminal_initialize(80, 25, (void*) virtualFramebuffer);
    enableLogging();

    VERBOSE("init: virtualFramebuffer at 0x%x\n", virtualFramebuffer);

    printf("\nStarting BlueberryOS\n");

    printf("Settings up System TSS ... ");
    tss_initialize();
    printf("\e[2;0m[OK]\e[0m\n");

    printf("Setting up GDT ...         ");
    gdt_initialize();
    printf("\e[2;0m[OK]\e[0m\n");

    printf("Flushing TSS ...           ");
    flush_tss();
    printf("\e[2;0m[OK]\e[0m\n");

    printf("Setting up IDT ...         ");
    idt_initialize();
    printf("\e[2;0m[OK]\e[0m\n");

    printf("Setting up Paging ...      ");
    paging_initialize();
    printf("\e[2;0m[OK]\e[0m\n");

    struct multiboot_tag_module modules[32];
    size_t moduleCount = 0;

    printf("Setting up Kernel Stack ...");
    uint32_t esp;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    set_kernel_stack(esp);
    printf("\e[2;0m[OK]\e[0m\n");

    if (eax != 0x36d76289) {
        ERROR("Failed to verify if bootloader has passed correct information\n");
    }


    VERBOSE("kernelstart: 0x%x(v) 0x%x(p)\n", p_to_v(KERNEL_START), KERNEL_START);
    VERBOSE("kernelend: 0x%x(v) 0x%x(p)\n", KERNEL_END, v_to_p(KERNEL_END));

    VERBOSE("Multiboot2 structure starting at 0x%x\n", ebx);

    for (tag = (struct multiboot_tag*) (ebx + 8);
       tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (struct multiboot_tag*) ((multiboot_uint8_t*) tag + ((tag->size + 7) & ~7)))
    {
        //VERBOSE("Tag 0x%x, Size 0x%x\n", tag->type, tag->size);

        switch (tag->type)
        {
            case MULTIBOOT_TAG_TYPE_CMDLINE:
                VERBOSE("Command line = '%s'\n", ((struct multiboot_tag_string *) tag)->string);
                break;
            case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
                VERBOSE("Boot loader name = '%s'\n", ((struct multiboot_tag_string *) tag)->string);
                break;
            case MULTIBOOT_TAG_TYPE_MODULE:
                struct multiboot_tag_module* tag_module = (struct multiboot_tag_module*) tag;

                VERBOSE("Module at 0x%x-0x%x. Command line '%s'\n",
                    p_to_v(tag_module->mod_start),
                    p_to_v(tag_module->mod_end),
                    tag_module->cmdline);

                memcpy(&modules[moduleCount++], tag_module, sizeof(struct multiboot_tag_module));
                break;
            case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
                VERBOSE("mem_lower = %uKB, mem_upper = %uKB\n",
                  ((struct multiboot_tag_basic_meminfo *) tag)->mem_lower,
                  ((struct multiboot_tag_basic_meminfo *) tag)->mem_upper);
                break;
            case MULTIBOOT_TAG_TYPE_BOOTDEV:
                VERBOSE("Boot device 0x%x,%u,%u\n",
                  ((struct multiboot_tag_bootdev *) tag)->biosdev,
                  ((struct multiboot_tag_bootdev *) tag)->slice,
                  ((struct multiboot_tag_bootdev *) tag)->part);
                break;

            /*
            case MULTIBOOT_TAG_TYPE_MMAP:
                {
                    multiboot_memory_map_t *mmap;

                    VERBOSE("mmap\n");

                    for (mmap = ((struct multiboot_tag_mmap *) tag)->entries;
                        (multiboot_uint8_t*) mmap < (multiboot_uint8_t *) tag + tag->size;
                        mmap = (multiboot_memory_map_t *) ((unsigned long) mmap + ((struct multiboot_tag_mmap *) tag)->entry_size)) {

                        VERBOSE(" base_addr = 0x%x%x,"
                        " length = 0x%x%x, type = 0x%x\n",
                        (unsigned) (mmap->addr >> 32),
                        (unsigned) (mmap->addr & 0xffffffff),
                        (unsigned) (mmap->len >> 32),
                        (unsigned) (mmap->len & 0xffffffff),
                        (unsigned) mmap->type);

                        unsigned int addr = p_to_v((unsigned int) (mmap->addr & 0xFFFFFFFF));
                        unsigned int len = (unsigned int) (mmap->len & 0xFFFFFFFF);

                        // Minimum requirement
                        if ((mmap->type == MULTIBOOT_MEMORY_AVAILABLE) && (addr + len > FRAME_START + 2*FRAME_4MB)) {
                            foundMemory = true;
                            VERBOSE("MMAP: Found mmap to use with size %d MB ranging from 0x%x to 0x%x\n", memorySize / FRAME_1MB, addr, addr + len);
                        }
                    }
                }
                break;
            */
        }
    }

    /*
    pagedirectory_t pd = page_directory;

    for (size_t j = 767; j < 1024; j++) {
        if (pd[j] & 1) {
            printf("   - %d for 0x%x - 0x%x: 0x%x\n", j, FRAME_4MB*j, FRAME_4MB*(j+1), pd[j]);
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
    */


    if (moduleCount == 0) {
        printf("\n\e[4;0m[FATAL]\e[0m No initrd found!\n");
        kabort();
    }

    for (size_t i = 0; i < moduleCount; i++) {

        struct multiboot_tag_module* module = &modules[i];
        module->mod_start = p_to_v(module->mod_start);
        module->mod_end = p_to_v(module->mod_end);

#ifdef _VERBOSE
        size_t moduleSize = module->mod_end - module->mod_start;
        VERBOSE("Module %d size: %d\n", i, moduleSize);
#endif
    }

    unsigned int destLen;
    void* dest = (void*) modules[0].mod_end + FRAME_4KB;
    uint32_t sourceLen = (uint32_t) modules[0].mod_end - (uint32_t) modules[0].mod_start;

    printf("Decompressing initrd ...   ");
    int status = tinf_gzip_uncompress(dest, &destLen, (void*) modules[0].mod_start, sourceLen);

    if (status == TINF_BUF_ERROR) {
        printf("\e[4;0m[ERROR]\e[0m\n");
        FATAL("Failed to decompress initrd: buffer error\n");
        kabort();

    } else if (status == TINF_DATA_ERROR) {

        VERBOSE("init: initrd wasn't tar.gz\n");
        printf("\e[2;0m[OK]\e[0m\n");
        // Not gzip
        loadInitrd(modules[0].mod_start, modules[0].mod_end);
    } else if (status == TINF_OK) {

        VERBOSE("init: initrd decompression sucessfull\n");
        printf("\e[2;0m[OK]\e[0m\n");
        loadInitrd((uint32_t) dest, (uint32_t) dest + destLen);
    }

#ifdef _VERBOSE
    displayDirectory(&rootDir, 0);
#endif



    printf("\n%s\n", titleText);

    printf("Welcome to BlueberryOS!\n");

    file_t* file;
    char* args[] = {"af", "Booga", 0};

    // TODO only run when there are no other processes
    file = getFile("/sbin/loop");
    if (!file) {
        ERROR("Failed to load application /sbin/loop\n");
        for (;;) {}
    }
    process_t* loop = newProcessArgs(file, args);
    (void) loop;

    file = getFile("/bin/shell");
    //file = getFile("/bin/test2");
    if (!file) {
        ERROR("Failed to load application /bin/shell\n");
        for (;;) {}
    }
    process_t* process = newProcessArgs(file, args);
    //printProcessInfo(process);
    (void) process;

    //printUserPagedirectory(process->pd);

    // Enable PIT interrupt
    irq_clear_mask(IRQ_TIMER);

    // Skip waiting
    //runProcess(loop);
    runProcess(process);


    for (;;) {
        asm("hlt");
    }

}


