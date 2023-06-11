
#ifndef KERNEL_FILE_H
#define KERNEL_FILE_H

#include <kernel/multiboot2.h>
#include <kernel/paging.h>

#include <stdint.h>
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

typedef struct {
    char magic[4];
    char class;
    char endianness;
    char version1;
    char targetABI;
    char ABIversion;
    char pad[7];
    uint16_t type;
    uint16_t targetArch;
    uint32_t version2;
    uint32_t entryPoint;
    uint32_t programTable;
    uint32_t sectionTable;
    uint32_t flags; /* May be unused */
    uint16_t headerSize;
    uint16_t programEntrySize;
    uint16_t programEntryCount;
    uint16_t sectionEntrySize;
    uint16_t sectionEntryCount;
    uint16_t sectionNamesIndex;
} elf_header_t;


bool isModuleELF(struct multiboot_tag_module* module);
file_t* loadFileFromMultiboot(struct multiboot_tag_module* module);

pagedirectory_t loadELFIntoMemory(struct multiboot_tag_module* module);
pagedirectory_t loadBinaryIntoMemory(struct multiboot_tag_module* module);

#endif /* KERNEL_FILE_H */

