
#include <kernel/file.h>
#include <kernel/memory.h>
#include <kernel/paging.h>
#include <kernel/errors.h>
#include <kernel/logging.h>

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




directory_t rootDir;




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

bool isFileELF(file_t* file) {

    // 32 bit ELF's header size is 52 bytes
    if (file->size < 53) {
        printf("Module not ELF: Too few bytes for ELF header\n");
        return false;
    }

    char magic[] = {0x7F, 'E', 'L', 'F'};
    if (strncmp(file->content, magic, 4) != 0) {
        printf("Module not ELF: Incorrect magic\n");
        return false;
    }

    // Check if file is 32-bit or 64-bit
    if (file->content[5] != 1) {
        printf("Unable to load file: Executable is ELF-64\n");
        return false;
    }

    elf_header_t elf_header = *((elf_header_t*) file->content);

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

pagedirectory_t loadELFIntoMemory(file_t* file) {

    // Copy kernel's pagedirectory to this pagedirectory
    pagedirectory_t pd = copy_system_pagedirectory();

    elf_header_t* elf_header = (elf_header_t*) file->content;

    /*
    for (size_t i = 1; i < elf_header->sectionEntryCount; i++) {
        section_header_t* section = getSectionEntry(elf_header, i);
        printf("Section name %d: '%s'. Type: '%d'\n", i, getSectionName(elf_header, i), section->type);
    }
    */

    VERBOSE("loadELFIntoMemory: ELF file has %d program header entries\n", elf_header->programEntryCount);
    for (size_t i = 0; i < elf_header->programEntryCount; i++) {
        program_header_t* program = getProgramEntry(elf_header, i);

        VERBOSE("loadELFIntoMemory: %d va: 0x%x, filesz: 0x%x, memsz: 0x%x, offset: 0x%x\n", i, program->vaddr, program->filesz, program->memsz, program->offset);

        // Check if program header is loadable
        if (program->type == 0x01) {

            if (program->vaddr >= 0xC0000000) {
                printf("[ERROR] Failed to load ELF becuase file goes into kernel-reserved space\n");
                for (;;) {}
                return pd;
            }

            /*
            if (program->memsz > FRAME_4KB) {
                printf("[ERROR] Can't load ELF program section because its size (%d) is greater than 4KB (4096)\n", program->memsz);
                for (;;) {}
                return pd;
            }
            */

            if (program->filesz > program->memsz) {
                printf("[ERROR] Can't load ELF program section because its filesz (%d) is greater than its memsz (%d)\n", program->filesz, program->memsz);
                for (;;) {}
                return pd;
            }

            // ceil(file->size / FRAME_4KB)
            unsigned int count = (program->filesz+FRAME_4KB-1)/FRAME_4KB;

            VERBOSE("loadELFIntoMemory: filesize: %d\n", program->filesz);
            VERBOSE("loadELFIntoMemory: Looping %d times\n", count);
            for (size_t j = 0; j < count; j++) {

                uint32_t offset = j*FRAME_4KB;
                VERBOSE("loadELFIntoMemory: Index %d with offset 0x%x\n", j, offset);

                // Allocate memory for program header
                pageframe_t pageframe = kalloc_frame();

                VERBOSE("loadELFIntoMemory: Allocated pageframe at 0x%x\n", pageframe);


                uint32_t physOffset;
                if (j == 0) {
                    //physOffset = program->vaddr % FRAME_4KB;
                    physOffset = program->vaddr & (FRAME_4KB-1);
                } else {
                    physOffset = 0x0;
                }

                VERBOSE("loadELFIntoMemory: Physical offset: 0x%x\n", physOffset);


                // Doesn't matter if the frames aren't consecutive
                /*
                uint32_t offset = j*FRAME_4KB;
                if ((uint32_t) pageframe != (uint32_t) firstpf + offset) {
                    printf("[ERROR] Can't load ELF because frames aren't consequtive\n");
                    for (;;) {}
                }
                */

                void* data = (void*) ((uint32_t) file->content + program->offset + offset);


                uint32_t size;
                if (i == (program->filesz+FRAME_4KB-1)/FRAME_4KB-1) {
                    //size = program->filesz % FRAME_4KB;
                    size = program->filesz & (FRAME_4KB - 1);
                } else {
                    size = FRAME_4KB;
                }

                VERBOSE("loadELFIntoMemory: Copying block with size %d bytes\n", size);

                // Set pageframe to zero with size of memsz
                memset(pageframe + physOffset, 0, program->memsz);

                //VERBOSE("loadELFIntoMemory: Offset was 0x%x, now 0x%x\n", program->offset, program->offset % FRAME_4KB);

                // Copy program header data to pageframe
                //memcpy(pageframe+offset, data, program->filesz);
                //memcpy(pageframe + program->offset, data, program->filesz);
                //memcpy(pageframe + physOffset, data, program->filesz);
                memcpy(pageframe + physOffset, data, size);

                /*
                printf("Program header %d\n", i);
                for (size_t j = 0; j < 60; j++) {
                    printf(" - %d: 0x%x, '%c'\n", j, ((char*) pageframe)[j], ((char*) pageframe)[j]);
                }
                */

                // Though this meant writable in memory, but probably not
                bool writable = program->flags & 0x2;

                VERBOSE("loadELFIntoMemory: Flags in program header: %d, %d\n", program->flags, writable);
                VERBOSE("loadELFIntoMemory: Mapping 0x%x to 0x%x\n", v_to_p((uint32_t) pageframe), program->vaddr + offset);

                // Map page
                // Set table to readwrite, but page can vary
                map_page_wtable_pd(pd, v_to_p((uint32_t) pageframe), program->vaddr + offset, writable, false, true, false);
            }

        } else {
            printf("[ERROR] Non-supported program header type %d\n", program->type);
            for (;;) {}
            return pd;
        }
    }

    return pd;
}

pagedirectory_t loadBinaryIntoMemory(file_t* file) {

    // Copy kernel's pagedirectory to this pagedirectory
    pagedirectory_t pd = copy_system_pagedirectory();

    // TODO very risky. Not guaranteed consecutive
    // ceil(file->size / FRAME_4KB)
    for (size_t i = 0; i < (file->size+FRAME_4KB-1)/FRAME_4KB; i++) {

        VERBOSE("Allocating new pageframe to copy executable data with index %d\n", i);

        uint32_t offset = i*FRAME_4KB;

        // Allocate memory for data
        pageframe_t pageframe = kalloc_frame();

        if ((uint32_t) pageframe != (uint32_t) file->content + offset) {
            printf("[ERROR] Can't load binary because frames aren't consequtive\n");
            for (;;) {}
        }

        uint32_t size;
        if (i == (file->size+FRAME_4KB-1)/FRAME_4KB-1) {
            //size = file->size % FRAME_4KB;
            size = file->size & (FRAME_4KB - 1);
        } else {
            size = FRAME_4KB;
        }

        // Copy data to pageframe
        memcpy((void*) ((uint32_t) pageframe + offset), (void*) (file->content + offset), size);

        // Map page
        map_page_pd(pd, v_to_p((uint32_t) pageframe + offset), 0x0 + offset, true, false);

        VERBOSE("Mapping 0x%x to 0x%x\n", v_to_p((uint32_t) pageframe + offset), 0x0 + offset);
    }


    return pd;
}

file_t* getFileFromParent(directory_t* parent, char* name) {

    for (size_t i = 0; i < parent->fileCount; i++) {
        if (strcmp(parent->files[i]->name, name) == 0) {
            return parent->files[i];
        }
    }

    return (file_t*) 0;
}

directory_t* getDirectoryFromParent(directory_t* parent, char* name) {

    for (size_t i = 0; i < parent->directoryCount; i++) {
        if (strcmp(parent->directories[i]->name, name) == 0) {
            directory_t* dir = parent->directories[i];

            if (dir->type == NORMAL_DIR) {
                return dir;
            } else if (dir->type == SYMBOLIC_LINK) {
                return dir->link;
            } else {
                printf("[ERROR] Unknown directory type %d\n", dir->type);
                return (directory_t*) 0;
            }
        }
    }

    return (directory_t*) 0;
}

directory_t* findParent(const char* filename, size_t* slash, bool init) {

    directory_t* parent = &rootDir;

    char name[MAX_NAME_LENGTH+1];
    size_t ni = 0;

    for (size_t i = 0; filename[i] != '\0'; i++) {

        //printf("Checking char '%c'. %d\n", header->filename[i], header->filename[i + 1]);
        if (filename[i] == '/') {
            name[ni] = '\0';
            ni = 0;

            // Directory since it end with /0
            if (filename[i+1] == '\0') {
                //printf("Directory, so stopping. Got parent %s for %s\n", parent->name, name);
                return parent;
            }

            *slash = i + 1;

            if (strcmp(name, "initrd") == 0 && init) {
                parent = &rootDir;
            } else if (strcmp(name, "") == 0) {
                parent = &rootDir;
            } else {
                //printf("name so far: %s\n", name);
                directory_t* p = getDirectoryFromParent(parent, name);
                if (!p) {
                    //printf("[ERROR] Directory '%s' not found in parent '%s'\n", name, parent->name);
                    return (directory_t*) 0;
                }

                parent = p;
                //printf("got parent: '%s'\n", parent->name);
            }


        } else {
            if (ni >= MAX_NAME_LENGTH) {
                printf("[ERROR] Max name length reached\n");
                for (;;) {}
            }

            name[ni++] = filename[i];
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
    directory_t* parent = findParent(header->filename, &slash, true);

    if (!parent) {
        printf("[ERROR] Failed to find parent of name %s\n", header->filename);
        for (;;) {}
    }

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


    // Create '.'
    createSymbolicDirectory(directory, directory, ".", directory->mode);

    // Create '..'
    createSymbolicDirectory(directory, parent, "..", parent->mode);
}

directory_t* createDirectory(directory_t* parent, char* name, char mode[4]) {

    // TODO LOT smaller
    directory_t* directory = kalloc_frame();
    memset(directory, 0, sizeof(directory_t));

    if (parent->directoryCount >= MAX_DIRECTORIES) {
        printf("[ERROR] Max directories reached\n");
        for (;;) {}
    }

    parent->directories[parent->directoryCount++] = directory;
    directory->parent = parent;


    size_t len = strlen(name);

    // Ignore slash at the end
    if (name[len-1] == '/') {
        len--;
    }

    if (len > MAX_NAME_LENGTH) {
        printf("[ERROR] Filename is too large\n");
        for (;;) {}
    }

    memcpy(directory->name, name, len);
    directory->name[len] = '\0';

    memcpy(directory->mode, mode, 3);
    directory->mode[3] = '\0';

    return directory;
}

directory_t* createSymbolicDirectory(directory_t* parent, directory_t* link, char* name, char mode[4]) {

    // TODO LOT smaller
    directory_t* directory = kalloc_frame();
    memset(directory, 0, sizeof(directory_t));

    if (parent->directoryCount >= MAX_DIRECTORIES) {
        printf("[ERROR] Max directories reached\n");
        for (;;) {}
    }

    parent->directories[parent->directoryCount++] = directory;
    directory->parent = parent;


    size_t len = strlen(name);

    // Ignore slash at the end
    if (name[len-1] == '/') {
        len--;
    }

    if (len > MAX_NAME_LENGTH) {
        printf("[ERROR] Filename is too large\n");
        for (;;) {}
    }

    memcpy(directory->name, name, len);
    directory->name[len] = '\0';

    memcpy(directory->mode, mode, 3);
    directory->mode[3] = '\0';

    directory->type = SYMBOLIC_LINK;
    directory->link = link;

    return directory;
}

void parseFile(tar_header_t* header) {

    uint32_t filesize = oct2bin(header->size, 11);

    /*
    if (filesize > FRAME_4KB) {
        printf("[ERROR] File-sizes larger than 4KB (4096) not supported yet. File '%s' has size %d\n", header->filename, filesize);
        for (;;) {}
    }
    */

    size_t len;

    size_t slash;
    directory_t* parent = findParent(header->filename, &slash, true);

    if (!parent) {
        printf("[ERROR] Failed to find parent of name %s\n", header->filename);
        for (;;) {}
    }

    if (parent->fileCount >= MAX_FILES) {
        printf("[ERROR] Max files reached\n");
        for (;;) {}
    }

    // TODO LOT smaller
    file_t* file = kalloc_frame();
    memset(file, 0, sizeof(file_t));

    //printf("Parsing file: %s\n", header->filename);

    // initrd: 6
    len = strlen(header->filename) - 6;
    if (len > MAX_FULL_PATH_LENGTH) {
        printf("[ERROR] Full path is too large\n");
        for (;;) {}
    }
    memcpy(file->fullpath, header->filename + 6, len + 1);

    parent->files[parent->fileCount++] = file;
    file->parent = parent;

    len = strlen(header->filename + slash);
    if (len > MAX_NAME_LENGTH) {
        printf("[ERROR] Filename is too large\n");
        for (;;) {}
    }

    memcpy(file->name, header->filename + slash, len+1);

    memcpy(file->mode, header->mode+4, 3);
    file->mode[3] = '\0';

    file->size = filesize;

    // TODO very risky. Not guaranteed consecutive
    // ceil(file->size / FRAME_4KB)
    //printf("Size is %d, looping %d times\n", filesize);
    for (size_t i = 0; i < (filesize+FRAME_4KB-1)/FRAME_4KB; i++) {

        pageframe_t pf = kalloc_frame();
        if (i == 0) {
            file->content = (char*) pf;
        }

        /*
        uint32_t offset = i*FRAME_4KB;

        if ((uint32_t) pf != (uint32_t) file->content + offset) {
            printf("[ERROR] Can't load file because frames aren't consequtive\n");
            for (;;) {}
        }

        uint32_t size;
        if (i == (filesize+FRAME_4KB-1)/FRAME_4KB-1) {
            size = filesize % FRAME_4KB;
        } else {
            size = FRAME_4KB;
        }
        */

    }

    memcpy((void*) file->content, (void*) ((uint32_t) header + 512), filesize);
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
    strcpy(rootDir.name, "/");
    memcpy(rootDir.mode, header->mode+4, 3);
    rootDir.mode[3] = '\0';

    rootDir.directoryCount = 0;
    rootDir.fileCount = 0;
    //printf("mode: %s\n", rootDir.mode);

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
}

file_t* getFile(char* filepath) {

    size_t slash;
    directory_t* dir = findParent(filepath, &slash, false);

    if (!dir) {
        return (file_t*) 0;
    }

    return getFileFromParent(dir, filepath + slash);
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




