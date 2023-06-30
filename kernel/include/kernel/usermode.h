
#ifndef KERNEL_USERMODE_H
#define KERNEL_USERMODE_H

#include <kernel/multiboot2.h>
#include <kernel/gdt.h>
#include <kernel/paging.h>
#include <kernel/file.h>
#include <kernel/idt.h>

#include <stdint.h>


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
    bool used;
    file_t* file;
    char mode[4];
    size_t position;
} fd_t;

typedef enum {
    RUNNING,
    BLOCKED_KEYBOARD,
    BLOCKED_WAITPID,
} process_state_t;

struct process;

typedef struct process {
    fd_t fds[MAX_FILE_DESCRIPTORS];
    struct process* children[MAX_CHILDREN];
    regs_t blocked_regs; // Registers when block happened
    regs_t regs;
    uint32_t esp;
    uint32_t eip;
    char name[PROCESS_MAX_NAME_LENGTH+1];
    pagedirectory_t pd;
    process_state_t state;
    struct process* parent;
    uint32_t indexInParent;
    file_t* file;
    directory_t* cwdir;
    uint32_t entryPoint;
    pageframe_t physical_stack;
    uint32_t virtual_stack;
    uint32_t virtual_stack_top;
    uint32_t id;
    bool initialized;
    bool overwritten;
} process_t;


extern void flush_tss(void);

void tss_initialize(void);
//void install_tss(struct GDT* source);
void install_tss(uint8_t* entryBytes);
void set_kernel_stack(uint32_t esp);
void use_system_tss(void);

process_t* findNextProcess(void);
process_t* getCurrentProcess(void);

process_t* newProcessAt(file_t* file, uint32_t pid);
process_t* newProcessArgs(file_t* file, char* args[]);
void setProcessArgs(process_t* process, char* args[]);
int overwriteArgs(process_t* process, char* filename, const char** args);

void terminateProcess(process_t* process, int status);
void runProcess(process_t* process);
void forkProcess(process_t* parent);
void switchProcess(void);
void runCurrentProcess(void);

void handleWaitpidBlock(process_t* process);
void handleKeyboardBlock(char c);

void printProcessInfo(process_t* process);

uint32_t processPush(process_t* process, uint32_t value);
uint32_t processPushStr(process_t* process, const char* str);

#endif /* KERNEL_USERMODE_H */

