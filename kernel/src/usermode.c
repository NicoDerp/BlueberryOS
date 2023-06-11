
#include <kernel/usermode.h>
#include <kernel/file.h>
#include <kernel/gdt.h>

#include <string.h>
#include <stdio.h>


void initialize_tss(tss_t* tss);

process_t processList[PROCESSES_MAX];
bool processUsed[PROCESSES_MAX];
int currentProcess;

tss_t sys_tss;
uint32_t kesp = 0;
uint8_t* kgdt;

void tss_initialize(void) {
    initialize_tss(&sys_tss);
}

void initialize_tss(tss_t* tss) {

    // Ensure tss is zeroed
    memset(tss, 0, sizeof(tss_t));

    tss->ss0 = 0x10;  /* Kernel data segment */
    tss->esp0 = kesp; /* Kernel stack pointer */

    tss->cs = 0x08;   /* Kernel code segment */

    /*
    sys_tss.es = 0x13;
    sys_tss.cs = 0x13;
    sys_tss.ss = 0x13;
    sys_tss.ds = 0x13;
    sys_tss.fs = 0x13;
    sys_tss.gs = 0x13;
    */

    //sys_tss.iomap = (unsigned short) sizeof(tss_t);
}

void install_tss_(tss_t* tss) {

    struct GDT source;

    source.access_byte = 0x89;
    source.flags = 0x0;
    source.base = (uint32_t) tss;
    source.limit = sizeof(tss_t);

    gdt_entry(kgdt, source);
}

void install_tss(uint8_t* gdt) {

    kgdt = gdt;
    install_tss_(&sys_tss);
}


process_t* newProcess(char* name, struct multiboot_tag_module* module) {

    process_t* process;
    bool found = false;
    for (size_t i = 0; i < PROCESSES_MAX; i++) {
        if (!processUsed[i]) {
            found = true;
            processUsed[i] = true;
            process = &processList[i];
            process->id = i;
            break;
        }
    }

    if (!found) {
        printf("[ERROR] Max processes reached!\n");
        return (process_t*) -1;
    }

    if (strlen(name) > PROCESS_MAX_NAME_LENGTH) {
        printf("[ERROR] Max process name reached!\n");
    }

    memcpy(process->name, name, PROCESS_MAX_NAME_LENGTH);

    // Just in case
    process->name[PROCESS_MAX_NAME_LENGTH] = 0;

    bool isELF = isModuleELF(module);
    if (isELF) {
        process->pd = loadELFIntoMemory(module);

        elf_header_t* elf_header = (elf_header_t*) module->mod_start;
        process->entryPoint = elf_header->entryPoint;
    } else {
        process->pd = loadBinaryIntoMemory(module);
        process->entryPoint = 0x0;
    }

    initialize_tss(&process->tss);

    process->physical_stack = (uint32_t) kalloc_frame();
    process->virtual_stack = FRAME_4MB + 0x1000;
    process->virtual_stack_top = process->virtual_stack + 0xF00;

    map_page_pd(process->pd, process->physical_stack, process->virtual_stack, true, false);

    return process;
}

void runProcess(process_t* process) {

    currentProcess = process->id;

    // Load process's TSS
    install_tss_(&process->tss);
    flush_tss();

    // Maybe need to reload GDT also, not sure

    // Load process's page directory
    loadPageDirectory(process->pd);

    // Enter usermode
    enter_usermode(process->entryPoint, process->virtual_stack_top);
}

void set_kernel_stack(uint32_t esp) {

    kesp = esp;

    // Setting ss0 just in case
    sys_tss.ss0 = 0x10;  /* Kernel data segment */
    sys_tss.esp0 = esp;
}


