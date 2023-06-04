
#ifndef KERNEL_FILE_H
#define KERNEL_FILE_H

#include <kernel/multiboot2.h>
#include <stdbool.h>

typedef enum {
    ASCII,
    ELF
} filetype_t;

typedef struct {
    char* name;
    filetype_t type;
    char* content;
} file_t;

bool isModuleELF(struct multiboot_tag_module* module);
file_t* loadFileFromMultiboot(struct multiboot_tag_module* module);

#endif /* KERNEL_FILE_H */

