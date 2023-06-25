
#include <kernel/file.h>
#include <kernel/memory.h>
#include <kernel/paging.h>
#include <kernel/errors.h>

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#include <stdio.h>


unsigned int oct2bin(unsigned char *str, int size);


 #define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })


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
    uint32_t align;
} program_header_t;

typedef struct {
    uint32_t name;
    uint32_t value;
    uint32_t size;
    char info;
    char other;
    uint16_t sectionIndex;
} symbol_table_t;




directory_t root;




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
        printf("Unable to load file: ELF is not executable. Type is 0x%x\n", elf_header.type);
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

    // Copy kernel's pagedirectory to this pagedirectory
    pagedirectory_t pd = copy_system_pagedirectory();

    elf_header_t* elf_header = (elf_header_t*) module->mod_start;

    /*
    for (size_t i = 1; i < elf_header->sectionEntryCount; i++) {
        section_header_t* section = getSectionEntry(elf_header, i);
        printf("Section name %d: '%s'. Type: '%d'\n", i, getSectionName(elf_header, i), section->type);
    }
    */

    printf("offset is %d bytes\n", elf_header->programTable);
    for (size_t i = 0; i < elf_header->programEntryCount; i++) {
        program_header_t* program = getProgramEntry(elf_header, i);
        printf("Type: %d, vaddr: 0x%x, filesz: 0x%x, memsz: 0x%x, align: 0x%x\n", program->type, program->vaddr, program->filesz, program->memsz, program->align);

        if (program->filesz > FRAME_4KB) {
            kerror("Can't load ELF because it contains a section over 4KB!\n");
        }

        // Check if program header is loadable
        if (program->type == 0x01) {

            if (program->memsz > FRAME_4KB) {
                printf("[ERROR] Can't load program header %d, because it is bigger than 4KB\n");
                return pd;
            }

            if (program->vaddr >= 0xC0000000) {
                printf("[ERROR] Failed to load ELF becuase file goes into kernel-reserved space\n");
            }

            // Allocate memory for program header
            pageframe_t pageframe = kalloc_frame();
            void* data = (void*) module->mod_start + program->offset;

            // TODO here is risk of buffer overflow
            // Same as mod 1024 but better
            //uint32_t offset = program->vaddr & 0x03FF;

            // Set pageframe to zero with size of memsz
            memset(pageframe, 0, program->memsz);

            // Copy program header data to pageframe
            //memcpy(pageframe+offset, data, program->filesz);
            memcpy(pageframe + program->offset, data, program->filesz);

            /*
            printf("Program header %d\n", i);
            for (size_t j = 0; j < 60; j++) {
                printf(" - %d: 0x%x, '%c'\n", j, ((char*) pageframe)[j], ((char*) pageframe)[j]);
            }
            */

            bool writable = program->flags & 0x2;

            // Map page
            map_page_pd(pd, v_to_p((uint32_t) pageframe), program->vaddr, writable, false);
            printf("Mapping 0x%x to 0x%x\n", v_to_p((uint32_t) pageframe), program->vaddr);
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
        //file->type = ELF;

        
    } else {
        //file->type = ASCII;
    }

    return file;
}

void debuga(const char* s) {
    for (size_t i = 0; s[i] != '\0'; i++) {

    }
}

directory_t* getDirectoryFromParent(directory_t* parent, char* name) {

    for (size_t i = 0; i < parent->directoryCount; i++) {
        if (strcmp(parent->directories[i]->name, name) == 0) {
            return parent->directories[i];
        }
    }

    return (directory_t*) -1;
}

directory_t* findParent(tar_header_t* header, size_t* slash) {

    directory_t* parent = &root;

    char name[MAX_NAME_LENGTH+1];
    size_t ni = 0;

    for (size_t i = 0; header->filename[i] != '\0'; i++) {

        //printf("Checking char '%c'. %d\n", header->filename[i], header->filename[i + 1]);
        if (header->filename[i] == '/') {
            name[ni] = '\0';
            ni = 0;

            // Directory since it end with /0
            if (header->filename[i+1] == '\0') {
                //printf("Directory, so stopping. Got parent %s for %s\n", parent->name, name);
                return parent;
            }

            *slash = i + 1;

            if (strcmp(name, "initrd") == 0) {
                parent = &root;
            } else {
                //printf("name so far: %s\n", name);
                directory_t* p = getDirectoryFromParent(parent, name);
                if (p == (directory_t*) -1) {
                    printf("[ERROR] Directory '%s' not found in parent '%s'\n", name, parent->name);
                    for (;;) {}
                }

                parent = p;
                //printf("got parent: '%s'\n", parent->name);
            }


        } else {
            if (ni >= MAX_NAME_LENGTH) {
                printf("[ERROR] Max name length reached\n");
                for (;;) {}
            }

            name[ni++] = header->filename[i];
        }
    }

    return parent;
}

void parseDirectory(tar_header_t* header) {

    // TODO LOT smaller
    directory_t* directory = kalloc_frame();
    memset(directory, 0, sizeof(directory_t));

    //printf("Parsing dir: %s\n", header->filename);

    size_t slash;
    directory_t* parent = findParent(header, &slash);

    if (parent->directoryCount >= MAX_DIRECTORIES) {
        printf("[ERROR] Max directories reached\n");
        for (;;) {}
    }

    parent->directories[parent->directoryCount++] = directory;

    directory->parent = parent;


    // Ignore slash at the end
    size_t len = strlen(header->filename + slash) - 1;

    if (len > MAX_NAME_LENGTH) {
        printf("[ERROR] Filename is too large\n");
        for (;;) {}
    }

    memcpy(directory->name, header->filename + slash, len);
    directory->name[len] = '\0';

    memcpy(directory->mode, header->mode+4, 3);
    directory->mode[3] = '\0';
}

void parseFile(tar_header_t* header) {

    // TODO LOT smaller
    file_t* file = kalloc_frame();
    memset(file, 0, sizeof(file_t));

    //printf("Parsing file: %s\n", header->filename);

    size_t slash;
    directory_t* parent = findParent(header, &slash);

    if (parent->fileCount >= MAX_FILES) {
        printf("[ERROR] Max files reached\n");
        for (;;) {}
    }

    parent->files[parent->fileCount++] = file;
    file->parent = parent;

    size_t len = strlen(header->filename + slash);
    if (len > MAX_NAME_LENGTH) {
        printf("[ERROR] Filename is too large\n");
        for (;;) {}
    }

    memcpy(file->name, header->filename + slash, len+1);

    memcpy(file->mode, header->mode+4, 3);
    file->mode[3] = '\0';

    file->size = oct2bin(header->size, 11);
    file->content = (char*) ((uint32_t) header + 512);
}

void displayDirectory(directory_t* dir, size_t space) {

    for (size_t i=0;i<space;i++) {putchar(' ');}
    printf("- %s: d%d, f%d\n", dir->name, dir->directoryCount, dir->fileCount);

    for (size_t i = 0; i < dir->fileCount; i++) {

        file_t* f = dir->files[i];

        for (size_t i=0;i<space;i++) {putchar(' ');}
        printf("  - %s\n", f->name);

        //for (size_t i=0;i<space;i++) {putchar(' ')}
        //printf("- mode: %s\n", file->mode);

        //for (size_t i=0;i<space;i++) {putchar(' ')}
        //printf("- content:\n%s\n", file->content);
    }

    for (size_t i = 0; i < dir->directoryCount; i++) {
        
        directory_t* d = dir->directories[i];

        displayDirectory(d, space+2);
    }
}

void loadInitrd(struct multiboot_tag_module* module) {

    tar_header_t* header;
    size_t offset = 512;

    header = (tar_header_t*) (uint32_t) module->mod_start;
    strcpy(root.name, "/");
    memcpy(root.mode, header->mode+4, 3);
    root.mode[3] = '\0';

    root.directoryCount = 0;
    root.fileCount = 0;
    //printf("mode: %s\n", root.mode);

    while (((uint32_t) module->mod_start + offset) <= (uint32_t) module->mod_end) {

        header = (tar_header_t*) (uint32_t) (module->mod_start + offset);

        if (memcmp(header->magic, "ustar", 5) != 0) {
            // EOF
            break;
        }

        /*
        printf("Name: '%s'\n", header->filename);
        printf("Size: '%s': '0x%x'\n", header->size, oct2bin(header->size, 11));
        printf("Magic: '%s'\n", header->magic);
        printf("Version: '%s'\n", header->version);
        printf("Prefix: '%s'\n", header->prefix);
        printf("linked: '%s'\n", header->linked);
        printf("type: %s\n", header->typeflag);
        */

        // Normal file
        if (header->typeflag[0] == '0') {

            // TODO parent
            parseFile(header);

        } else if (header->typeflag[0] == '5') {

            parseDirectory(header);

        } else {
            printf("[ERROR] Unsupported tar header type '%s'\n", header->typeflag);
            for (;;) {}
        }

        uint32_t size = oct2bin(header->size, 11);
        offset += 512 + (-size % 512) + size;

        //printf("offset: %d\n", offset);

    }

    displayDirectory(&root, 0);
}

unsigned int oct2bin(unsigned char* str, int size) {
    unsigned int n = 0;
    unsigned char* c = str;
    while (size-- > 0) {
        n *= 8;
        n += *c - '0';
        c++;
    }

    return n;
}




