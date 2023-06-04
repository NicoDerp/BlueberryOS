
#include <kernel/file.h>
#include <kernel/memory.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <stdio.h>


bool isModuleELF(struct multiboot_tag_module* module) {

    size_t size = module->mod_end - module->mod_start;

    // 32 bit ELF's header size is 52 bytes
    if (size < 53) {
        printf("Module not ELF: Too few bytes for ELF header\n");
        return false;
    }

    char magic[] = {0x7F, 'E', 'L', 'F'};
    if (strncmp((char*) module->mod_start, magic, 4) != 0) {
        printf("Module not ELF: Incorrect header\n");
        return false;
    }

    return true;

}

file_t* loadFileFromMultiboot(struct multiboot_tag_module* module) {

    // TODO smaller
    file_t* file = (file_t*) kalloc_frame();
    bool isELF = isModuleELF(module);
    printf("is elf: %d\n", isELF);
    return file;
}

