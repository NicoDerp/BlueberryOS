
#include <kernel/file.h>
#include <kernel/memory.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#include <stdio.h>


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
    uint32_t headerTable;
    uint32_t sectionTable;
    uint32_t flags; /* May be unused */
    uint16_t headerSize;
    uint16_t headerEntrySize;
    uint16_t headerEntryCount;
    uint16_t sectionEntrySize;
    uint16_t sectionEntryCount;
    uint16_t sectionNamesIndex;
} file_header_t;

typedef struct {
    uint32_t nameOffset;
    uint32_t type;
    uint32_t flags;
    uint32_t addr;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t alignment;
    uint32_t entrySize;
} section_header_t;

static inline section_header_t* getSectionHeader(file_header_t* file_header) {
    return (section_header_t*) ((int) file_header + file_header->sectionTable);
}

static inline section_header_t* getSectionEntry(file_header_t* file_header, size_t index) {
    return &getSectionHeader(file_header)[index];
}

static inline char* getNamesEntry(file_header_t* file_header) {
    if (file_header->sectionNamesIndex == 0) {
        return NULL;
    }

    section_header_t* section = getSectionEntry(file_header, file_header->sectionNamesIndex);
    char* namesEntry = (char*) ((int)file_header + section->offset);
    return namesEntry;
}

static inline char* getSectionName(file_header_t* file_header, size_t index) {
    if (file_header->sectionNamesIndex == 0) {
        return NULL;
    }

    section_header_t* section = getSectionEntry(file_header, index);
    section_header_t* namesSection = getSectionEntry(file_header, file_header->sectionNamesIndex);
    char* namesEntry = (char*) ((int)file_header + namesSection->offset);
    return namesEntry + section->nameOffset;
}

bool isModuleELF(struct multiboot_tag_module* module) {

    size_t size = module->mod_end - module->mod_start;

    // 32 bit ELF's header size is 52 bytes
    if (size < 53) {
        printf("Module not ELF: Too few bytes for ELF header\n");
        return false;
    }

    char magic[] = {0x7F, 'E', 'L', 'F'};
    if (strncmp((char*) module->mod_start, magic, 4) != 0) {
        printf("Module not ELF: Incorrect magic\n");
        return false;
    }

    // Check if file is 32-bit or 64-bit
    if (((char*) module->mod_start)[5] != 1) {
        printf("Unable to load file: Executable is ELF-64\n");
        return false;
    }

    file_header_t file_header = *((file_header_t*) module->mod_start);

    // Check endianness
    if (file_header.endianness != 1) {
        printf("Unable to load file: Executable not made for little-endian\n");
        return false;
    }
    
    // Check if ELF version is correct
    if (file_header.version1 != 1) {
        printf("Unable to load file: ELF version is not 1\n");
        return false;
    }

    // Check if file is executable
    if (file_header.type != 0x02) {
        printf("Unable to load file: ELF is not executable\n");
        return false;
    }

    // Check if executable is made for x86
    if (file_header.targetArch != 0x03) {
        printf("Unable to load file: Executable is not made for x86\n");
        return false;
    }

    return true;

}

file_t* loadFileFromMultiboot(struct multiboot_tag_module* module) {

    // TODO smaller
    file_t* file = (file_t*) kalloc_frame();

    bool isELF = isModuleELF(module);
    printf("is elf: %d\n", isELF);

    if (isELF) {
        file->type = ELF;

        file_header_t* file_header = (file_header_t*) module->mod_start;

        for (size_t i = 1; i < file_header->sectionEntryCount; i++) {
            printf("Section name %d: '%s'\n", i, getSectionName(file_header, i));
        }

        
    } else {
        file->type = ASCII;
    }

    return file;
}

