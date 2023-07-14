
#include <kernel/file.h>
#include <kernel/memory.h>
#include <kernel/paging.h>
#include <kernel/errors.h>
#include <kernel/usermode.h>
#include <kernel/logging.h>

#include <asm-generic/errno-values.h>
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

pageframe_t currentPageframe = 0;
uint32_t currentLocation;


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
        ERROR("Module not ELF: Too few bytes for ELF header\n");
        return false;
    }

    char magic[] = {0x7F, 'E', 'L', 'F'};
    if (strncmp(file->content, magic, 4) != 0) {
        ERROR("Module not ELF: Incorrect magic\n");
        return false;
    }

    // Check if file is 32-bit or 64-bit
    if (file->content[5] != 1) {
        ERROR("Unable to load file: Executable is ELF-64\n");
        return false;
    }

    elf_header_t elf_header = *((elf_header_t*) file->content);

    // Check endianness
    if (elf_header.endianness != 1) {
        ERROR("Unable to load file: Executable not made for little-endian\n");
        return false;
    }
    
    // Check if ELF version is correct
    if (elf_header.version1 != 1) {
        printf("Unable to load file: ELF version is not 1\n");
        return false;
    }

    // Check if file is executable
    if (elf_header.type != 0x02) {
        ERROR("Unable to load file: ELF is not executable. Type is 0x%x\n", elf_header.type);
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
                ERROR("Failed to load ELF becuase file goes into kernel-reserved space\n");
                for (;;) {}
                return pd;
            }

            /*
            if (program->memsz > FRAME_4KB) {
                ERROR("Can't load ELF program section because its size (%d) is greater than 4KB (4096)\n", program->memsz);
                for (;;) {}
                return pd;
            }
            */

            if (program->filesz > program->memsz) {
                ERROR("Can't load ELF program section because its filesz (%d) is greater than its memsz (%d)\n", program->filesz, program->memsz);
                for (;;) {}
                return pd;
            }

            // ceil(file->size / FRAME_4KB)
            unsigned int fzcount = (program->filesz+FRAME_4KB-1)/FRAME_4KB;
            unsigned int mzcount = (program->memsz+FRAME_4KB-1)/FRAME_4KB;

            VERBOSE("loadELFIntoMemory: filesz: %d, memsz: %d\n", program->filesz, program->memsz);
            VERBOSE("loadELFIntoMemory: Looping %d times with %d fzcount\n", mzcount, fzcount);
            for (size_t j = 0; j < mzcount; j++) {

                uint32_t offset = j*FRAME_4KB;
                VERBOSE("loadELFIntoMemory: Index %d with offset 0x%x\n", j, offset);

                // TODO space for optimizatoin
                // if memsz == filesz, then you don't need to allocate new pageframe!
                // Just use the same!

                // Allocate memory for program header
                pageframe_t pageframe = kalloc_frame();

                VERBOSE("loadELFIntoMemory: Allocated pageframe at 0x%x\n", pageframe);

                /*
                uint32_t msize;
                if (j == mzcount-1) {
                    msize = program->memsz & (FRAME_4KB - 1);
                } else {
                    msize = FRAME_4KB;
                }

                // Set pageframe to zero with msize with has 4KB size, but for last has
                //  how much is left
                memset(pageframe, 0, msize);
                */

                memset(pageframe, 0, FRAME_4KB);

                // If there is data to copy
                if (j < fzcount) {

                    uint32_t physOffset;
                    if (j == 0) {
                        physOffset = program->vaddr & (FRAME_4KB-1);
                    } else {
                        physOffset = 0x0;
                    }

                    VERBOSE("loadELFIntoMemory: Physical offset: 0x%x\n", physOffset);


                    // Doesn't matter if the frames aren't consecutive
                    /*
                    uint32_t offset = j*FRAME_4KB;
                    if ((uint32_t) pageframe != (uint32_t) firstpf + offset) {
                        ERROR("Can't load ELF because frames aren't consequtive\n");
                        for (;;) {}
                    }
                    */

                    void* data = (void*) ((uint32_t) file->content + program->offset + offset);


                    uint32_t size;
                    if (j == (program->filesz+FRAME_4KB-1)/FRAME_4KB-1) {
                        //size = program->filesz % FRAME_4KB;
                        size = program->filesz & (FRAME_4KB - 1);
                    } else {
                        size = FRAME_4KB;
                    }

                    if (physOffset + size > FRAME_4KB) {
                        ERROR("Size of section exceeds pageframe\n");
                        for (;;) {}
                    }

                    VERBOSE("loadELFIntoMemory: Copying block with size %d bytes\n", size);

                    // Copy program header data to pageframe
                    memcpy(pageframe + physOffset, data, size);
                }

                //VERBOSE("loadELFIntoMemory: Offset was 0x%x, now 0x%x\n", program->offset, program->offset % FRAME_4KB);

                /*
                printf("Program header %d\n", i);
                for (size_t j = 0; j < 60; j++) {
                    printf(" - %d: 0x%x, '%c'\n", j, ((char*) pageframe)[j], ((char*) pageframe)[j]);
                }
                */

                bool writable = program->flags & 0x2;

                VERBOSE("loadELFIntoMemory: Flags in program header: %d, %d\n", program->flags, writable);
                VERBOSE("loadELFIntoMemory: Mapping 0x%x to 0x%x\n", v_to_p((uint32_t) pageframe), program->vaddr + offset);

                // Map page
                // Set table to readwrite, but page can vary
                map_page_wtable_pd(pd, v_to_p((uint32_t) pageframe), program->vaddr + offset, writable, false, true, false);
            }

        } else {
            ERROR("Non-supported program header type %d\n", program->type);
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
            ERROR("Can't load binary because frames aren't consequtive\n");
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

file_t* getFileFromParent(directory_t* parent, char* name, bool redirectSymbolic) {

    for (size_t i = 0; i < parent->fileCount; i++) {
        if (strcmp(parent->files[i]->name, name) == 0) {

            file_t* file = parent->files[i];

            if (file->type == SYMBOLIC_FILE) {
                if (redirectSymbolic)
                    return file->link;

                return file;
            }
            else if (file->type == REGULAR_FILE) {
                return file;
            }
            else {
                ERROR("Unknown file type %d\n", file->type);
                for (;;) {}
                return (file_t*) 0;
            }
        }
    }

    return (file_t*) 0;
}

directory_t* getDirectoryFromParent(directory_t* parent, char* name, bool redirectSymbolic) {

    size_t len = strlen(name);
    char nameBuf[len+1];
    memcpy(nameBuf, name, len+1);
    if (name[len-1] == '/')
        nameBuf[len-1] = '\0';

    for (size_t i = 0; i < parent->directoryCount; i++) {

        if (strcmp(parent->directories[i]->name, nameBuf) == 0) {

            directory_t* dir = parent->directories[i];

            if (dir->type == SYMBOLIC_DIR) {
                if (redirectSymbolic)
                    return dir->link;

                return dir;
            }
            else if (dir->type == REGULAR_DIR) {
                return dir;
            }
            else
            {
                ERROR("Unknown directory type %d\n", dir->type);
                for (;;) {}
                return (directory_t*) 0;
            }
        }
    }

    return (directory_t*) 0;
}

directory_t* findParent(directory_t* parent, const char* filename, size_t* slash, bool init) {

    //directory_t* parent = &rootDir;
    //directory_t* parent = (directory_t*) 0;

    char name[MAX_NAME_LENGTH+1];
    size_t ni = 0;

    // If we never set it
    *slash = 0;

    VERBOSE("findParent: Path: %s\n", filename);
    for (size_t i = 0; filename[i] != '\0'; i++) {

        //printf("Checking char '%c'. %d\n", header->filename[i], header->filename[i + 1]);
        if (filename[i] == '/') {
            name[ni] = '\0';
            ni = 0;

            // Directory since it end with /0
            if (filename[i+1] == '\0') {
                if (i == 0) {
                    return &rootDir;
                }

                //printf("Directory, so stopping. Got parent %s for %s\n", parent->name, name);
                return parent;
            }

            *slash = i + 1;

            if (strcmp(name, "initrd") == 0 && init) {
                parent = &rootDir;
            } else if (strcmp(name, "") == 0) {
                parent = &rootDir;
            } else {
                // If parent isn't set then the path is: abc/, which means local
                // But since we pass parent this means that this directory can't exist
                //  (we don't deal with enviroment variables)
                if (!parent) {
                    VERBOSE("findParent: got local directory, but enviroment variables aren't supported yet\n");
                    for (;;) {}
                    return (directory_t*) 0;
                }

                //printf("name so far: %s\n", name);
                directory_t* p = getDirectoryFromParent(parent, name, true);
                if (!p) {
                    if (init) {
                        FATAL("Directory '%s' not found in parent '%s' during initialization\n", name, parent->name);
                        kabort();
                    }
                    return (directory_t*) 0;
                }

                parent = p;
                //printf("got parent: '%s'\n", parent->name);
            }


        } else {
            if (ni >= MAX_NAME_LENGTH) {
                ERROR("Max name length reached\n");
                if (init)
                    kabort();

                return (directory_t*) 0;
            }

            name[ni++] = filename[i];
        }
    }

    return parent;
}

void parseDirectory(tar_header_t* header) {

    VERBOSE("parseDirectory: parsing %s\n", header->filename);

    directory_t* directory = (directory_t*) kmalloc(sizeof(directory_t));
    memset(directory, 0, sizeof(directory_t));

    //printf("Parsing dir: %s\n", header->filename);

    size_t len;
    size_t slash;
    directory_t* parent = findParent((directory_t*) 0, header->filename, &slash, true);

    if (!parent) {
        ERROR("Failed to find parent of name %s\n", header->filename);
        for (;;) {}
    }

    if (parent->directoryCount >= MAX_DIRECTORIES) {
        ERROR("Max directories reached with count %d\n", parent->directoryCount);
        for (;;) {}
    }

    parent->directories[parent->directoryCount++] = directory;

    directory->parent = parent;

    len = strlen(header->filename) - 6;
    if (len > MAX_FULL_PATH_LENGTH) {
        ERROR("Full path is too large\n");
        for (;;) {}
    }

    // Cut away trailing /
    memcpy(directory->fullpath, header->filename+6, len-1);
    directory->fullpath[len-1] = '\0';

    // Ignore slash at the end
    len = strlen(header->filename + slash) - 1;

    if (len > MAX_NAME_LENGTH) {
        ERROR("Filename is too large\n");
        for (;;) {}
    }

    memcpy(directory->name, header->filename + slash, len);
    directory->name[len] = '\0';
    directory->mode = oct2bin(header->mode, 7);
    directory->owner = rootUser;
    directory->group = rootPGroup;

    // Create '.'
    createSymbolicDirectory(directory, directory, ".", directory->mode, rootUser, rootPGroup, NULL);

    // Create '..'
    createSymbolicDirectory(directory, parent, "..", parent->mode, rootUser, rootPGroup, NULL);

    VERBOSE("parseDirectory: end\n");
}

directory_t* createDirectory(directory_t* parent, char* name, uint32_t mode, user_t* owner, group_t* group, int* errnum) {

    directory_t* directory = (directory_t*) kmalloc(sizeof(directory_t));
    memset(directory, 0, sizeof(directory_t));

    if (parent->directoryCount >= MAX_DIRECTORIES) {
        ERROR("Max directories reached with count %d\n", parent->directoryCount);
        if (errnum)
            *errnum = ENOMEM;
        return (directory_t*) 0;
    }

    parent->directories[parent->directoryCount++] = directory;
    directory->parent = parent;

    size_t len = strlen(name);
    size_t parentLen = strlen(parent->fullpath);

    // Ignore slash at the end
    if (name[len-1] == '/') {
        len--;
    }

    if (len > MAX_NAME_LENGTH) {
        ERROR("Filename is too large\n");
        if (errnum)
            *errnum = ENAMETOOLONG;
        return (directory_t*) 0;
    }

    if (parentLen + len + 1 > MAX_FULL_PATH_LENGTH) {
        ERROR("Fullpath is too large!\n");
        if (errnum)
            *errnum = ENAMETOOLONG;
        return (directory_t*) 0;
    }

    memcpy(directory->name, name, len+1);

    memcpy(directory->fullpath, parent->fullpath, parentLen);

    if (parent->fullpath[parentLen-1] != '/') {
        directory->fullpath[parentLen] = '/';
        parentLen++;
    }

    memcpy(directory->fullpath + parentLen, name, len);
    directory->fullpath[parentLen + len] = '\0';

    directory->mode = mode;
    directory->type = REGULAR_DIR;
    directory->owner = owner;
    directory->group = group;

    // Create '.'
    createSymbolicDirectory(directory, directory, ".", directory->mode, owner, group, NULL);

    // Create '..'
    createSymbolicDirectory(directory, parent, "..", parent->mode, parent->owner, parent->group, NULL);

    return directory;
}

directory_t* createSymbolicDirectory(directory_t* parent, directory_t* link, char* name, uint32_t mode, user_t* owner, group_t* group, int* errnum) {

    if (parent->directoryCount >= MAX_DIRECTORIES) {
        ERROR("Max directories reached with count %d\n", parent->directoryCount);
        if (errnum)
            *errnum = ENOMEM;
        return (directory_t*) 0;
    }

    directory_t* directory = (directory_t*) kmalloc(sizeof(directory_t));
    memset(directory, 0, sizeof(directory_t));

    parent->directories[parent->directoryCount++] = directory;
    directory->parent = parent;


    size_t len = strlen(name);
    size_t parentLen = strlen(parent->fullpath);

    // Ignore slash at the end
    if (name[len-1] == '/') {
        len--;
    }

    if (len > MAX_NAME_LENGTH) {
        ERROR("Filename is too large\n");
        if (errnum)
            *errnum = ENAMETOOLONG;
        return (directory_t*) 0;
    }

    if (parentLen + len + 1 > MAX_FULL_PATH_LENGTH) {
        ERROR("Fullpath is too large!\n");
        if (errnum)
            *errnum = ENAMETOOLONG;
        return (directory_t*) 0;
    }

    memcpy(directory->name, name, len+1);

    memcpy(directory->fullpath, parent->fullpath, parentLen);

    if (parent->fullpath[parentLen-1] != '/') {
        directory->fullpath[parentLen] = '/';
        parentLen++;
    }

    memcpy(directory->fullpath + parentLen, name, len);
    directory->fullpath[parentLen + len] = '\0';


    directory->mode = mode;
    directory->type = SYMBOLIC_DIR;
    directory->link = link;
    directory->owner = owner;
    directory->group = group;

    return directory;
}

file_t* createFile(directory_t* parent, char* name, uint32_t mode, user_t* user, group_t* group, int* errnum) {

    if (parent->fileCount >= MAX_FILES) {
        ERROR("Max files reached\n");
        return (file_t*) 0;
    }

    file_t* file = (file_t*) kmalloc(sizeof(file_t));
    memset(file, 0, sizeof(file_t));



    size_t len = strlen(name);
    size_t parentLen = strlen(parent->fullpath);

    // Ignore slash at the end
    if (name[len-1] == '/') {
        len--;
    }

    if (len > MAX_NAME_LENGTH) {
        ERROR("Filename is too large\n");
        if (errnum)
            *errnum = ENAMETOOLONG;
        return (file_t*) 0;
    }

    if (parentLen + len + 1 > MAX_FULL_PATH_LENGTH) {
        ERROR("Fullpath is too large!\n");
        if (errnum)
            *errnum = ENAMETOOLONG;
        return (file_t*) 0;
    }

    memcpy(file->name, name, len+1);

    memcpy(file->fullpath, parent->fullpath, parentLen);

    if (parent->fullpath[parentLen-1] != '/') {
        file->fullpath[parentLen] = '/';
        parentLen++;
    }

    memcpy(file->fullpath + parentLen, name, len);
    file->fullpath[parentLen + len] = '\0';



    parent->files[parent->fileCount++] = file;
    file->parent = parent;

    file->mode = mode;
    file->size = 0;
    file->type = REGULAR_FILE;
    file->owner = user;
    file->group = group;

    // ceil(file->size / FRAME_4KB)
    //printf("Size is %d, looping %d times\n", filesize);

    file->frames = 0;
    file->content = (char*) 0;

    return file;
}

void parseFile(tar_header_t* header) {

    VERBOSE("parseFile: parsing %s\n", header->filename);

    uint32_t filesize = oct2bin(header->size, 11);

    /*
    if (filesize > FRAME_4KB) {
        ERROR("File-sizes larger than 4KB (4096) not supported yet. File '%s' has size %d\n", header->filename, filesize);
        for (;;) {}
    }
    */

    size_t len;

    size_t slash;
    directory_t* parent = findParent((directory_t*) 0, header->filename, &slash, true);

    if (!parent) {
        ERROR("Failed to find parent of name %s\n", header->filename);
        for (;;) {}
    }

    if (parent->fileCount >= MAX_FILES) {
        ERROR("Max files reached\n");
        for (;;) {}
    }

    file_t* file = (file_t*) kmalloc(sizeof(file_t));
    memset(file, 0, sizeof(file_t));

    //printf("Parsing file: %s\n", header->filename);

    // initrd: 6
    len = strlen(header->filename) - 6;
    if (len > MAX_FULL_PATH_LENGTH) {
        ERROR("Full path is too large\n");
        for (;;) {}
    }
    memcpy(file->fullpath, header->filename + 6, len + 1);

    parent->files[parent->fileCount++] = file;
    file->parent = parent;

    len = strlen(header->filename + slash);
    if (len > MAX_NAME_LENGTH) {
        ERROR("Filename is too large\n");
        for (;;) {}
    }

    memcpy(file->name, header->filename + slash, len+1);

    file->mode = oct2bin(header->mode, 7);
    file->size = filesize;
    file->type = REGULAR_FILE;
    file->owner = rootUser;
    file->group = rootPGroup;

    // ceil(file->size / FRAME_4KB)
    //printf("Size is %d, looping %d times\n", filesize);

    size_t count = (filesize+FRAME_4KB-1)/FRAME_4KB;
    file->frames = count;
    VERBOSE("parseFile: Allocating %d consecutive frames\n", count);
    file->content = (char*) kalloc_frames(count);
    memcpy((void*) file->content, (void*) ((uint32_t) header + 512), filesize);

    VERBOSE("parseFile: end\n");
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

void loadInitrd(uint32_t tar_start, uint32_t tar_end) {

    tar_header_t* header;
    size_t offset = 512;

    header = (tar_header_t*) (uint32_t) tar_start;
    strcpy(rootDir.fullpath, "/");
    strcpy(rootDir.name, "/");
    rootDir.mode = oct2bin(header->mode, 7);
    rootDir.directoryCount = 0;
    rootDir.fileCount = 0;
    rootDir.owner = rootUser;
    rootDir.group = rootPGroup;

    // Create '.'
    createSymbolicDirectory(&rootDir, &rootDir, ".", rootDir.mode, rootUser, rootPGroup, NULL);

    // Create '..'
    createSymbolicDirectory(&rootDir, &rootDir, "..", rootDir.mode, rootUser, rootPGroup, NULL);


    while ((tar_start + offset) <= tar_end) {

        header = (tar_header_t*) (tar_start + offset);

        VERBOSE("mod_start at 0x%x\n", tar_start);
        VERBOSE("offset is %d\n", offset);
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

            parseFile(header);

        } else if (header->typeflag[0] == '5') {

            parseDirectory(header);

        } else {
            ERROR("Unsupported tar header type '%s'\n", header->typeflag);
            for (;;) {}
        }

        uint32_t size = oct2bin(header->size, 11);
        VERBOSE("loadInitrd: Header size is '%s'\n", header->size);
        offset += 512 + (-size % 512) + size;

        //printf("offset: %d\n", offset);

    }
}

directory_t* getDirectory(char* path) {

    size_t slash;
    directory_t* parent = findParent((directory_t*) 0, path, &slash, false);

    if (!parent) {
        VERBOSE("getDirectory: no parent\n");
        return (directory_t*) 0;
    }

    if (strcmp(parent->name, path) == 0) {
        return parent;
    }

    VERBOSE("getDirectory: Got parent %s\n", parent->fullpath);

    return getDirectoryFromParent(parent, path + slash, true);
}

file_t* getFile(char* filepath) {

    size_t slash;
    directory_t* parent = findParent((directory_t*) 0, filepath, &slash, false);

    if (!parent) {
        return (file_t*) 0;
    }

    return getFileFromParent(parent, filepath + slash, true);
}

void changeDirectoryOwner(directory_t* dir, user_t* owner, group_t* group, bool recursive) {

    if (!group) {
        FATAL("Group is NULL!\n");
        return;
    }

    dir->owner = owner;
    dir->group = group;

    directory_t* d = getDirectoryFromParent(dir, ".", false);
    if (!d) {
        FATAL("Directory has no '.' directory!\n");
        return;
    }

    d->owner = owner;
    d->group = group;

    if (!recursive)
        return;

    for (size_t i = 0; i < dir->directoryCount; i++) {

        d = dir->directories[i];
        if (strcmp(d->name, ".") == 0 || strcmp(d->name, "..") == 0)
            continue;

        changeDirectoryOwner(d, owner, group, true);
    }

    for (size_t i = 0; i < dir->fileCount; i++) {

        file_t* f = dir->files[i];
        f->owner = owner;
        f->group = group;
    }
}

directory_t* getDirectoryFrom(directory_t* dir, char* path, bool redirectSymbolic) {

    size_t slash;
    directory_t* parent = findParent(dir, path, &slash, false);

    if (!parent) {
        VERBOSE("getDirectory: no parent\n");
        return (directory_t*) 0;
    }

    if (strcmp(parent->name, path) == 0) {
        return parent;
    }

    VERBOSE("getDirectory: Got parent %s\n", parent->fullpath);

    return getDirectoryFromParent(parent, path + slash, redirectSymbolic);
}

file_t* getFileFrom(directory_t* dir, char* filepath, bool redirectSymbolic) {

    size_t slash;
    directory_t* parent = findParent(dir, filepath, &slash, false);

    if (!parent) {
        return (file_t*) 0;
    }

    return getFileFromParent(parent, filepath + slash, redirectSymbolic);
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




