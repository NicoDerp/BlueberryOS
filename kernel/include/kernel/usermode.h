
#ifndef KERNEL_USERMODE_H
#define KERNEL_USERMODE_H

#include <kernel/multiboot2.h>
#include <kernel/gdt.h>
#include <kernel/paging.h>
#include <kernel/file.h>
#include <kernel/idt.h>

#include <sys/stat.h>
#include <stdint.h>
#include <shadow.h>

#include <pwd.h>
#include <grp.h>


#define PROCESS_MAX_NAME_LENGTH (256+64)
#define PROCESSES_MAX 32

#define PROCESS_TIME 0x00F0
//#define PROCESS_TIME 0xFFFF
#define STACK_TOP_OFFSET 0xF00
#define STACK_TOP_INDEX (STACK_TOP_OFFSET/4-1)

#define MAX_ARGS 64
#define MAX_ARG_LENGTH 256

#define MAX_FILE_DESCRIPTORS 64

#define MAX_CHILDREN 32

#define MAX_ENVIROMENT_VARIABLES 32
#define MAX_VARIABLE_KEY_LENGTH 32
#define MAX_VARIABLE_VALUE_LENGTH 64

#define MAX_USERS             4
#define MAX_USERNAME_LENGTH   64
#define MAX_PASSWORD_LENGTH   64
#define MAX_USER_GROUPS  4

#define MAX_GROUPS            4
#define MAX_GROUP_NAME_LENGTH 64
#define MAX_GROUP_MEMBERS     8

#define MMAP_START_ADDRESS    0x400000

#define MIN_STDIN_BUFFER_SIZE 32
#define MAX_STDIN_BUFFER_SIZE 128
#define STDIN_SETBACK         32


struct user;
struct kgroup;


typedef struct {
    uint32_t prev_tss; // The previous TSS - with hardware task switching these form a kind of backward linked list.
    uint32_t esp0;     // The stack pointer to load when changing to kernel mode.
    uint32_t ss0;      // The stack segment to load when changing to kernel mode.
    // Everything below here is unused.
    uint32_t esp1; // esp and ss 1 and 2 would be used when switching to rings 1 or 2.
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
    //uint32_t ssp;
} __attribute__((packed)) tss_t;

typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t ecx;
} regs_t;

typedef struct {
    size_t position;
    size_t fd;
    uint32_t pointer;
    int flags;
    bool active;
} pfd_t;

typedef enum {
    RUNNING,
    ZOMBIE,
    BLOCKED_KEYBOARD,
    BLOCKED_WAITPID,
} process_state_t;

typedef struct {
    char key[MAX_VARIABLE_KEY_LENGTH+1];
    char value[MAX_VARIABLE_VALUE_LENGTH+1];
    bool active;
} env_variable_t;

typedef struct user {
    char name[MAX_USERNAME_LENGTH+1];
    char password[MAX_PASSWORD_LENGTH+1];
    struct kgroup* groups[MAX_USER_GROUPS];
    directory_t* home;
    directory_t* iwdir;
    file_t* program;
    uint32_t uid;
    bool root;
    bool active;
} user_t;

typedef struct kgroup {
    char name[MAX_GROUP_NAME_LENGTH+1];
    struct user* members[MAX_GROUP_MEMBERS];
    uint32_t gid;
    bool active;
} group_t;

struct process;

typedef struct process {
    pfd_t pfds[MAX_FILE_DESCRIPTORS];
    struct process* children[MAX_CHILDREN];
    regs_t blocked_regs; // Registers when block happened
    regs_t regs;
    env_variable_t variables[MAX_ENVIROMENT_VARIABLES];
    pagedirectory_t pd;
    process_state_t state;
    struct process* parent;
    pageframe_t physical_stack;
    file_t* file;
    directory_t* cwdir;
    user_t* owner;
    uint32_t indexInParent;
    uint32_t entryPoint;
    uint32_t esp;
    uint32_t eip;
    uint32_t virtual_stack;
    uint32_t virtual_stack_top;
    uint32_t id;
    uint32_t stdinIndex;
    uint32_t stdinSize;
    char* stdinBuffer;
    bool initialized;
    bool overwritten;
    char name[PROCESS_MAX_NAME_LENGTH+1];
} process_t;


extern void flush_tss(void);

extern user_t* rootUser;
extern user_t* currentUser;
extern group_t* rootPGroup;

extern process_t processes[];

void tss_initialize(void);
//void install_tss(struct GDT* source);
void install_tss(uint8_t* entryBytes);
void set_kernel_stack(uint32_t esp);
void use_system_tss(void);

bool resolveProcessAddress(process_t* process, const void* virtaddr, uint32_t count, bool needrw);
bool resolveZeroProcessAddress(process_t* process, const void* virtaddr);

process_t* findNextProcess(void);
process_t* getCurrentProcess(void);

process_t* newProcessArgs(file_t* file, char* args[], user_t* user);
void setProcessArgs(process_t* process, char* args[]);
int overwriteArgs(process_t* process, char* filename, const char** args, int* errnum);

void terminateProcess(process_t* process, int status);
void runProcess(process_t* process);
void forkProcess(process_t* parent);
void switchProcess(void);
void runCurrentProcess(void);

bool fileAccessAllowed(process_t* process, file_t* file, unsigned int check);
bool directoryAccessAllowed(process_t* process, directory_t* dir, uint32_t check);

bool userInGroup(user_t* user, group_t* group);
void addUserToGroup(user_t* user, group_t* group);
group_t* createGroup(char* name);
user_t* createUser(char* name, char* password, bool createHome, bool root);

user_t* getUserByUID(uint32_t uid);
user_t* getUserByName(char* name);
group_t* getGroupByGID(uint32_t gid);

int getPasswdStructR(uint32_t uid, struct passwd* pwd, char* buffer, uint32_t bufsize, struct passwd** result);
int getGroupStructR(uint32_t gid, struct group* grp, char* buffer, size_t bufsize, struct group** result);
int getSpwdStructR(char* name, struct spwd* spw, char* buffer, uint32_t bufsize, struct spwd** result);

int mmapProcess(process_t* process, uint32_t address, uint32_t length, int prot, int flags, int fd, uint32_t offset, int* errnum);
int munmapProcess(process_t* process, uint32_t address, uint32_t length, int* errnum);

env_variable_t* getEnvVariable(process_t* process, const char* key);
int setEnvVariable(process_t* process, const char* key, const char* value, bool overwrite);
int unsetEnvVariable(process_t* process, const char* key);
file_t* getFileWEnv(process_t* process, char* path);

int readProcessFd(process_t* process, char* buf, size_t count, unsigned int fd);
int writeProcessFd(process_t* process, char* buf, size_t count, unsigned int fd, int* errnum);

int openProcessFile(process_t* process, char* pathname, uint32_t flags, uint32_t permissions, int* errnum);
int closeProcessFd(process_t* process, unsigned int fd);
pfd_t* getProcessPfd(process_t* process, unsigned int fd);

int getDirectoryEntries(process_t* process, int fd, char* buf, size_t nbytes, uint32_t* basep);
int statPath(process_t* process, char* path, struct stat* buf, bool redirectSymbolic, int* errnum);

void handleWaitpid(process_t* process);
void handleWaitpidBlock(process_t* process);
void handleKeyboardBlock(char c);

void printProcessInfo(process_t* process);

uint32_t processPush(process_t* process, uint32_t value);
uint32_t processPushStr(process_t* process, const char* str);

#endif /* KERNEL_USERMODE_H */

