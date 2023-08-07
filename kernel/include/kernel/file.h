
#ifndef KERNEL_FILE_H
#define KERNEL_FILE_H

#include <kernel/multiboot2.h>
#include <kernel/paging.h>

#include <stdint.h>
#include <stdbool.h>



#define MAX_DIRECTORIES      32
#define MAX_FILES            32
#define MAX_NAME_LENGTH      64
#define MAX_FULL_PATH_LENGTH 256

/* ELF program header types */
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6

struct directory;
struct file;

struct user;
struct kgroup;

typedef enum {
    REGULAR_DIR,
    SYMBOLIC_DIR
} dirtype_t;

typedef enum {
    REGULAR_FILE,
    SYMBOLIC_FILE
} filetype_t;

typedef struct directory {
    char fullpath[MAX_FULL_PATH_LENGTH+1];
    char name[MAX_NAME_LENGTH+1];
    uint32_t mode;
    struct directory* parent;

    // TODO should be linked. Waste especially if type is SYMBOLIC_LINK
    struct directory* directories[MAX_DIRECTORIES];
    struct file* files[MAX_FILES];

    struct user* owner;
    struct kgroup* group;

    uint32_t directoryCount;
    uint32_t fileCount;
    dirtype_t type;

    // Only used when type is SYMBOLIC_LINK
    struct directory* link;
} directory_t;

typedef struct file {
    char fullpath[MAX_FULL_PATH_LENGTH+1];
    char name[MAX_NAME_LENGTH+1];
    uint32_t mode;
    struct directory* parent;

    struct user* owner;
    struct kgroup* group;

    size_t size;
    filetype_t type;
    uint32_t frames;

    // Only used when type is SYMBOLIC_LINK
    struct file* link;

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

typedef struct
{
    char filename[100];
    unsigned char mode[8];
    unsigned char uid[8];
    unsigned char gid[8];
    unsigned char size[12];
    unsigned char mtime[12];
    unsigned char checksum[8];
    unsigned char typeflag[1];
    char linked[100];
    char magic[6];
    char version[2];
    char ownerName[32];
    char groupName[32];
    char deviceMajor[8];
    char deviceMinor[8];
    char prefix[155];
} tar_header_t;

#define P_READ    (1<<2)
#define P_WRITE   (1<<1)
#define P_EXECUTE (1<<0)
#define P_ALL     7

extern directory_t rootDir;


bool isFileELF(file_t* file);

void loadInitrd(uint32_t tar_start, uint32_t tar_end);

directory_t* getDirectory(char* path);
file_t* getFile(char* filepath);

void changeDirectoryOwner(directory_t* dir, struct user* owner, struct kgroup* group, bool recursive);
directory_t* getDirectoryFromParent(directory_t* parent, char* name, bool redirectSymbolic);

directory_t* getDirectoryFrom(directory_t* dir, char* path, bool redirectSymbolic);
file_t* getFileFrom(directory_t* dir, char* filepath, bool redirectSymbolic);

directory_t* findParent(directory_t* parent, const char* filename, size_t* slash, bool init);

directory_t* createDirectory(directory_t* parent, char* name, uint32_t mode, struct user* owner, struct kgroup* group, int* errnum);
directory_t* createSymbolicDirectory(directory_t* parent, directory_t* link, char* name, uint32_t mode, struct user* owner, struct kgroup* group, int* errnum);

file_t* createFile(directory_t* parent, char* name, uint32_t mode, struct user* user, struct kgroup* group, int* errnum);

void displayDirectory(directory_t* dir, size_t space);

pagedirectory_t loadELFIntoMemory(file_t* file);
pagedirectory_t loadBinaryIntoMemory(file_t* file);

#endif /* KERNEL_FILE_H */

