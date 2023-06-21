
#ifndef KERNEL_USERMODE_H
#define KERNEL_USERMODE_H

#include <kernel/multiboot2.h>
#include <kernel/gdt.h>
#include <kernel/paging.h>

#include <stdint.h>


#define PROCESS_MAX_NAME_LENGTH 128
#define PROCESSES_MAX 32

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
    regs_t regs;
    uint32_t esp;
    char name[PROCESS_MAX_NAME_LENGTH+1];
    pagedirectory_t pd;
    uint32_t entryPoint;
    uint32_t physical_stack;
    uint32_t virtual_stack;
    uint32_t virtual_stack_top;
    int id;
} process_t;


extern void flush_tss(void);

void tss_initialize(void);
//void install_tss(struct GDT* source);
void install_tss(uint8_t* entryBytes);
void set_kernel_stack(uint32_t esp);
void use_system_tss(void);

process_t* getCurrentProcess(void);
process_t* newProcess(char* name, struct multiboot_tag_module* module);
void runProcess(process_t* processID);
void switchProcess(void);

#endif /* KERNEL_USERMODE_H */

