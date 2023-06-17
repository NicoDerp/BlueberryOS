
#include <kernel/file.h>
#include <kernel/memory.h>
#include <kernel/paging.h>

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#include <stdio.h>


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

typedef struct {
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t filesz;
    uint32_t memsz;
    uint32_t flags;
    uint32_t align; /* TODO */
} program_header_t;

typedef struct {
    uint32_t name;
    uint32_t value;
    uint32_t size;
    char info;
    char other;
    uint16_t sectionIndex;
} symbol_table_t;

static inline section_header_t* getSectionHeader(elf_header_t* elf_header) {
    return (section_header_t*) ((int) elf_header + elf_header->sectionTable);
}

static inline section_header_t* getSectionEntry(elf_header_t* elf_header, size_t index) {
    return &getSectionHeader(elf_header)[index];
}

static inline program_header_t* getProgramHeader(elf_header_t* elf_header) {
    return (program_header_t*) ((int) elf_header + elf_header->programTable);
}

static inline program_header_t* getProgramEntry(elf_header_t* elf_header, size_t index) {
    return &getProgramHeader(elf_header)[index];
}

static inline char* getNamesEntry(elf_header_t* elf_header) {
    if (elf_header->sectionNamesIndex == 0) {
        return NULL;
    }

    section_header_t* section = getSectionEntry(elf_header, elf_header->sectionNamesIndex);
    char* namesEntry = (char*) ((int)elf_header + section->offset);
    return namesEntry;
}

static inline char* getSectionName(elf_header_t* elf_header, size_t index) {
    if (elf_header->sectionNamesIndex == 0) {
        return NULL;
    }

    section_header_t* section = getSectionEntry(elf_header, index);
    section_header_t* namesSection = getSectionEntry(elf_header, elf_header->sectionNamesIndex);
    char* namesEntry = (char*) ((int)elf_header + namesSection->offset);
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

    elf_header_t elf_header = *((elf_header_t*) module->mod_start);

    // Check endianness
    if (elf_header.endianness != 1) {
        printf("Unable to load file: Executable not made for little-endian\n");
        return false;
    }
    
    // Check if ELF version is correct
    if (elf_header.version1 != 1) {
        printf("Unable to load file: ELF version is not 1\n");
        return false;
    }

    // Check if file is executable
    if (elf_header.type != 0x02) {
        printf("Unable to load file: ELF is not executable\n");
        return false;
    }

    // Check if executable is made for x86
    if (elf_header.targetArch != 0x03) {
        printf("Unable to load file: Executable is not made for x86\n");
        return false;
    }

    return true;

}

pagedirectory_t loadELFIntoMemory(struct multiboot_tag_module* module) {

    pagedirectory_t pd = new_pagedirectory(true, false);

    elf_header_t* elf_header = (elf_header_t*) module->mod_start;

    /*
    for (size_t i = 1; i < elf_header->sectionEntryCount; i++) {
        section_header_t* section = getSectionEntry(elf_header, i);
        printf("Section name %d: '%s'. Type: '%d'\n", i, getSectionName(elf_header, i), section->type);
    }
    */

    for (size_t i = 0; i < elf_header->programEntryCount; i++) {
        program_header_t* program = getProgramEntry(elf_header, i);
        printf("Type: %d, vaddr: 0x%x, memsz: 0x%x, align: 0x%x\n", program->type, program->vaddr, program->memsz, program->align);

        // Check if program header is loadable
        if (program->type == 0x01) {

            if (program->memsz > FRAME_4KB) {
                printf("[ERROR] Can't load program header %d, because it is bigger than 4KB\n");
                return pd;
            }

            // Allocate memory for program header
            pageframe_t pageframe = kalloc_frame();
            void* data = (void*) module->mod_start + program->offset;

            // Set pageframe to zero with size of memsz
            memset(pageframe, 0, program->memsz);

            // Copy program header data to pageframe
            memcpy(pageframe, data, program->filesz);

            // Map page
            map_page_pd(pd, p_to_v((uint32_t) pageframe), program->vaddr, true, false);
        }
    }

    return pd;
}

pagedirectory_t loadBinaryIntoMemory(struct multiboot_tag_module* module) {

    // Copy kernel's pagedirectory to this pagedirectory
    pagedirectory_t pd = copy_system_pagedirectory();

    size_t module_size = module->mod_end - module->mod_start;

    // ceil(module_size / FRAME_4KB)
    for (size_t i = 0; i < (module_size+FRAME_4KB-1)/FRAME_4KB; i++) {

        printf("Allocating new pageframe to copy executable data with index %d\n", i);

        uint32_t offset = i*FRAME_4KB;

        // Allocate memory for data
        pageframe_t pageframe = kalloc_frame();

        // Copy data to pageframe
        memcpy((void*) ((uint32_t) pageframe + offset), (void*) (module->mod_start + offset), module_size);

        char* byte = (char*) pageframe;
        printf("byte 0: 0x%x\n", ((unsigned int) byte[1]) & 0xFF);

        // Map page
        map_page_pd(pd, v_to_p((uint32_t) pageframe + offset), 0x0 + offset, true, false);
        printf("Mapping 0x%x to 0x%x\n", v_to_p((uint32_t) pageframe + offset), 0x0 + offset);
    }


    return pd;
}

file_t* loadFileFromMultiboot(struct multiboot_tag_module* module) {

    // TODO smaller
    file_t* file = (file_t*) kalloc_frame();

    bool isELF = isModuleELF(module);
    printf("is elf: %d\n", isELF);

    if (isELF) {
        file->type = ELF;

        
    } else {
        file->type = ASCII;
    }

    return file;
}

