
#include <kernel/usermode.h>
#include <kernel/file.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>

#include <string.h>
#include <stdio.h>


extern void enter_usermode(uint32_t addr, uint32_t stack_ptr, regs_t regs);

void initialize_tss(tss_t* tss);


process_t processes[PROCESSES_MAX];
bool processUsed[PROCESSES_MAX];
size_t currentProcessID;

tss_t sys_tss;

void tss_initialize() {

    // Ensure tss is zeroed
    memset(&sys_tss, 0, sizeof(tss_t));

    sys_tss.ss0 = 0x10;  /* Kernel data segment */
    sys_tss.esp0 = 0x0;  /* Kernel stack pointer */

    sys_tss.cs = 0x08;   /* Kernel code segment */

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

void install_tss(uint8_t* gdt) {

    struct GDT source;

    source.access_byte = 0x89;
    source.flags = 0x0;
    source.base = (uint32_t) &sys_tss;
    source.limit = sizeof(tss_t);

    gdt_entry(gdt, source);
}

process_t* findNextProcess(void) {

    process_t* process;
    bool found = false;
    size_t i = currentProcessID + 1;

    // Find next process to run
    do {
        if (i >= PROCESSES_MAX) {
            i = 0;
        }

        // Process for that ID is active
        if (processUsed[i]) {
            process = &processes[i];
            found = true;
            break;
        }

        i++;
    }
    while (i != currentProcessID);

    if (!found) {
        if (!processUsed[currentProcessID]) {
            printf("[ERROR] No current processes?? Idk what to do now\n");
            for (;;) {}
            return (process_t*) -1;
        }

        process = &processes[currentProcessID];
    }

    //printf("Found process %d\n", process->id);

    return process;
}

process_t* getCurrentProcess(void) {
    return &processes[currentProcessID];
}

process_t* newProcess(char* name, struct multiboot_tag_module* module) {

    process_t* process;
    bool found = false;
    for (size_t i = 0; i < PROCESSES_MAX; i++) {
        if (!processUsed[i]) {
            found = true;
            processUsed[i] = true;
            process = &processes[i];
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

    process->physical_stack = (uint32_t) kalloc_frame();
    memset((void*) process->physical_stack, 0, FRAME_4KB);

    process->virtual_stack = 4*FRAME_4KB; // Place stack at some place 4KB
    process->virtual_stack_top = process->virtual_stack + STACK_TOP_OFFSET;

    printf("Physical stack at 0x%x. Virtual at 0x%x\n", process->physical_stack, process->virtual_stack);
    map_page_pd(process->pd, v_to_p(process->physical_stack), process->virtual_stack, true, false);

    process->eip = process->entryPoint;
    process->esp = process->virtual_stack_top;

    processPush(process, 69);
    processPush(process, 49);

    return process;
}

void terminateProcess(process_t* process, int status) {

    (void) status;

    processUsed[process->id] = false;

    // Don't need to clear process because it will get initialized
    // memset(process, 0, sizeof(process_t))

    // TODO free memory and shit
}

void runProcess(process_t* process) {

    currentProcessID = process->id;

    // Load process's page directory
    loadPageDirectory(process->pd);

    // Enter usermode
    enter_usermode(process->entryPoint, process->virtual_stack_top, process->regs);
}

void switchProcess(void) {

    //printf("Next process\n");
    // Simple round robin

    process_t* process = findNextProcess();

    //printf("Next process is process '%s' with id %d\n", process->name, process->id);

    currentProcessID = process->id;

    // Load process's page directory
    loadPageDirectory(process->pd);

    // Reset PIT count
    pit_set_count(PROCESS_TIME);

    // Enter usermode
    enter_usermode(process->eip, process->esp, process->regs);
}

void processPush(process_t* process, uint32_t value) {

    size_t offset = (process->virtual_stack_top - process->esp) / 4;
    ((uint32_t*) process->physical_stack)[STACK_TOP_INDEX - offset] = value;
    process->esp -= 4;

}

void set_kernel_stack(uint32_t esp) {

    // Setting ss0 just in case
    sys_tss.ss0 = 0x10;  /* Kernel data segment */
    sys_tss.esp0 = esp;
}

