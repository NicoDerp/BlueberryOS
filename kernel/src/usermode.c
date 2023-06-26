
#include <kernel/usermode.h>
#include <kernel/file.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/logging.h>

#include <string.h>
#include <stdio.h>


extern void enter_usermode(uint32_t addr, uint32_t stack_ptr, regs_t regs);

void initialize_tss(tss_t* tss);


process_t processes[PROCESSES_MAX];
uint32_t currentProcessID;

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
    uint32_t i = currentProcessID + 1;

    // Find next process to run
    do {
        process = &processes[i];

        if (process->initialized && process->state == RUNNING) {
            found = true;
            break;
        }

        i++;
        if (i >= PROCESSES_MAX) {
            i = 0;
        }

    }
    while (i != currentProcessID);

    if (!found) {
        if (!processes[currentProcessID].initialized) {
            printf("[ERROR] No current processes?? Idk what to do now\n");
            for (;;) {}
            return (process_t*) 0;
        }

        process = &processes[currentProcessID];

        if (process->state != RUNNING) {
            printf("[ERROR] Only one process to run and that is blocked\n");
        }
    }

    //printf("Found process %d\n", process->id);

    return process;
}

process_t* getCurrentProcess(void) {
    return &processes[currentProcessID];
}

process_t* newProcessAt(file_t* file, uint32_t pid) {

    VERBOSE("Creating new process with id %d\n", index);

    if (pid >= PROCESSES_MAX) {
        printf("[ERROR] Can't create process with pid outside maximum limit\n");
        for (;;) {}
    }

    process_t* process = &processes[pid];
    memset(process, 0, sizeof(process_t));

    process->id = pid;

    size_t len = strlen(file->fullpath);
    if (len > PROCESS_MAX_NAME_LENGTH) {
        printf("[ERROR] Max process name reached!\n");
        len = PROCESS_MAX_NAME_LENGTH;
        for (;;) {}
    }

    memcpy(process->name, file->fullpath, len);
    process->name[len] = '\0';

    bool isELF = isFileELF(file);
    if (isELF) {
        process->pd = loadELFIntoMemory(file);

        elf_header_t* elf_header = (elf_header_t*) file->content;
        process->entryPoint = elf_header->entryPoint;
    } else {
        process->pd = loadBinaryIntoMemory(file);
        process->entryPoint = 0x0;
    }

    process->physical_stack = kalloc_frame();
    memset((void*) process->physical_stack, 0, FRAME_4KB);

    process->virtual_stack = 4*FRAME_4KB; // Place stack at some place 4KB
    process->virtual_stack_top = process->virtual_stack + STACK_TOP_OFFSET;

    map_page_pd(process->pd, v_to_p((uint32_t) process->physical_stack), process->virtual_stack, true, false);

    VERBOSE("Physical stack at 0x%x. Virtual at 0x%x\n", process->physical_stack, process->virtual_stack);

    process->state = RUNNING;
    process->eip = process->entryPoint;
    process->esp = process->virtual_stack_top;
    process->file = file;
    process->initialized = true;
    process->parent = (process_t*) 0;

    return process;
}

process_t* newProcess(file_t* file) {

    bool found = false;
    uint32_t index;
    for (index = 0; index < PROCESSES_MAX; index++) {
        if (!processes[index].initialized) {
            found = true;
            break;
        }
    }

    if (!found) {
        printf("[ERROR] Max processes reached!\n");
        for (;;) {}
        return (process_t*) 0;
    }

    return newProcessAt(file, index);
}

process_t* newProcessArgs(file_t* file, const char* args[]) {

    process_t* process = newProcess(file);
    setProcessArgs(process, args);

    return process;
}

void setProcessArgs(process_t* process, const char* args[]) {

    int argCount;
    for (argCount = 0; args[argCount] != 0; argCount++) {}

    //printf("argCount: %d\n", argCount);
    if (argCount >= MAX_ARGS) {
        printf("[ERROR] Can't create process with arg count %d because it exceeds limit of %d\n", argCount, MAX_ARGS);
        for (;;) {}
    }

    uint32_t strPointers[argCount];

    // Push strings
    for (int i = 0; i < argCount; i++) {
        VERBOSE("Pushing str[%d] as '%s'\n", i, args[i]);

        strPointers[i] = processPushStr(process, args[i]);
    }

    // Push string pointers
    for (int i = 0; i < argCount; i++) {
        int j = argCount - i - 1;

        VERBOSE("Pushing argv[%d] as 0x%x\n", j, strPointers[j]);

        processPush(process, strPointers[j]);

    }

    uint32_t esp = process->esp;

    // argv must be zero-terminated
    processPush(process, 0);

    // argv
    processPush(process, esp);

    // Push argc
    processPush(process, argCount);
}

int overwriteArgs(process_t* process, char* filename, const char* args[]) {

    // TODO use enviromental variables n shi and current directory
    file_t* file = getFile(filename);
    if (!file) {
        return -1;
    }

    // Free pagedirectory since that is kalloc'ed in loadELF/Binary IntoMemory
    freeUserPagedirectory(process->pd);

    memset(process->children, 0, sizeof(process_t*) * MAX_CHILDREN);
    memset(&process->regs, 0, sizeof(regs_t));

    if (process->id >= PROCESSES_MAX) {
        printf("[ERROR] Can't create process with pid outside maximum limit\n");
        return -1;
    }

    size_t len = strlen(file->fullpath);
    if (len > PROCESS_MAX_NAME_LENGTH) {
        printf("[ERROR] Max process name reached!\n");
        len = PROCESS_MAX_NAME_LENGTH;
    }

    memcpy(process->name, file->fullpath, len);
    process->name[len] = '\0';

    bool isELF = isFileELF(file);
    if (isELF) {
        process->pd = loadELFIntoMemory(file);

        elf_header_t* elf_header = (elf_header_t*) file->content;
        process->entryPoint = elf_header->entryPoint;
    } else {
        process->pd = loadBinaryIntoMemory(file);
        process->entryPoint = 0x0;
    }

    // Don't allocate new, just use old
    //process->physical_stack = (uint32_t) kalloc_frame();

    memset((void*) process->physical_stack, 0, FRAME_4KB);

    process->virtual_stack = 4*FRAME_4KB; // Place stack at some place 4KB
    process->virtual_stack_top = process->virtual_stack + STACK_TOP_OFFSET;

    map_page_pd(process->pd, v_to_p((uint32_t) process->physical_stack), process->virtual_stack, true, false);

    VERBOSE("Physical stack at 0x%x. Virtual at 0x%x\n", process->physical_stack, process->virtual_stack);

    process->state = RUNNING;
    process->eip = process->entryPoint;
    process->esp = process->virtual_stack_top;
    process->file = file;
    process->initialized = true;
    process->parent = (process_t*) 0;


    // Add args
    setProcessArgs(process, args);

    return 0;
}

void terminateProcess(process_t* process, int status) {

    (void) status;

    process->initialized = false;

    /*
    if (process->parent) {
        process->parent->children[process->indexInParent] = 0;
    }
    */

    // Don't need to clear process because it will get initialized
    // memset(process, 0, sizeof(process_t))

    // TODO free memory and shit
    freeUserPagedirectory(process->pd);
    kfree_frame(process->physical_stack);
}

void runProcess(process_t* process) {

    currentProcessID = process->id;

    // Load process's page directory
    loadPageDirectory(process->pd);

    // Reset PIT count
    pit_set_count(PROCESS_TIME);

    // Enter usermode
    enter_usermode(process->entryPoint, process->virtual_stack_top, process->regs);
}

void forkProcess(process_t* parent) {

    bool found = false;
    size_t index;
    for (index = 0; index < MAX_CHILDREN; index++) {
        if (!parent->children[index]) {
            found = true;
            break;
        }
    }

    if (!found) {
        printf("[ERROR] Max children reached for process %d\n", parent->id);

        // Set to indicate error
        parent->regs.eax = -1;
        return;
    }


    process_t* child = newProcess(parent->file);

    // Copy stack
    memcpy((void*) child->physical_stack, (void*) parent->physical_stack, FRAME_4KB);

    // Copy registers
    memcpy(&child->regs, &parent->regs, sizeof(regs_t));

    // Copy other parameters
    child->parent = parent;
    child->esp = parent->esp;
    child->eip = parent->eip;
    child->indexInParent = index;
    parent->children[index] = child;

    parent->regs.eax = child->id;
    child->regs.eax = 0;

    // TODO in future also copy pagedirectory for things like malloc
}

void switchProcess(void) {

    //printf("Next process\n");
    // Simple round robin

    process_t* process = findNextProcess();
    currentProcessID = process->id;

    //printf("Found next process: %d, %s\n", process->id, process->name);
    /*
    for (size_t i = 0; i < PROCESSES_MAX; i++) {
        process_t* p = &processes[i];
        if (p->initialized) {
            printf("%d: %d %s\n", i, p->id, p->name);
        }
    }
    for (;;){}
    */

    // Load process's page directory
    loadPageDirectory(process->pd);

    // Reset PIT count
    pit_set_count(PROCESS_TIME);

    // Enter usermode
    enter_usermode(process->eip, process->esp, process->regs);
}

void runCurrentProcess(void) {

    process_t* process = &processes[currentProcessID];

    if (!process->initialized) {
        printf("[ERROR] Kernel tried to execute non-initialized currently running process\n");
        for (;;) {}
    }

    // Load process's page directory
    loadPageDirectory(process->pd);

    // Enter usermode
    enter_usermode(process->eip, process->esp, process->regs);
}

void handleKeyboardBlock(char c) {

    for (size_t i = 0; i < PROCESSES_MAX; i++) {
        process_t* process = &processes[i];

        if (process->state == BLOCKED_KEYBOARD) {

            // Quick fix
            loadPageDirectory(process->pd);

            char* buf = (char*) process->blocked_regs.ecx;

            buf[process->regs.eax] = c;   // Write to buffer
            process->blocked_regs.edx--;  // Count requested
            process->regs.eax++;          // Return value: bytes read

            // If counter is zero, then unblock
            if (process->blocked_regs.edx == 0) {
                process->state = RUNNING;

                if (process->parent && process->parent->state == BLOCKED_WAITPID) {
                    process->parent->state = RUNNING;

                    // Sucess
                    process->parent->regs.eax = process->id;

                    int* status = (int*) process->parent->blocked_regs.ebx;
                    if (status != NULL) {
                        printf("[INFO] Status from waitpid used but is unimplemented\n");
                        loadPageDirectory(process->parent->pd);
                        *status = 0; // TODO unimplemented WIFEXITED n shi
                    }
                }

                // Loop process
                /*
                if (currentProcessID == 0) {
                    switchProcess();
                }
                */
            }
        }
    }

    loadPageDirectory(processes[currentProcessID].pd);
}

void printProcessInfo(process_t* process) {

    printf("Process info:\n");
    printf(" - Name: '%s'\n", process->name);
    printf(" - Id: %d\n", process->id);
    printf(" - Entry point: 0x%x\n", process->entryPoint);
    printf(" - Stack: 0x%x\n", process->physical_stack);
    printf(" - Pagetables:\n");

    for (size_t j = 0; j < 767; j++) {
        if (process->pd[j] & 1) {
            printf("   - %d: 0x%x\n", j, process->pd[j]);
            pagetable_t pagetable = (pagetable_t) p_to_v(process->pd[j] & 0xFFFFF000);
            for (size_t k = 0; k < 1024; k++) {
                if (pagetable[k] & 1) {
                    printf("     - Page %d: 0x%x\n", k, pagetable[k]);
                }
            }
        }
    }
}

uint32_t processPush(process_t* process, uint32_t value) {

    uint32_t ret = (process->virtual_stack_top - process->esp);
    size_t offset = ret / 4;
    ((uint32_t*) process->physical_stack)[STACK_TOP_INDEX - offset] = value;
    process->esp -= 4;

    return process->esp + 4;
}

uint32_t processPushStr(process_t* process, const char* str) {

    uint32_t ret;
    uint32_t offset = process->virtual_stack_top - process->esp;
    size_t len = strlen(str) + 1;

    if (len >= MAX_ARG_LENGTH) {
        printf("[ERROR] Can't push arg with length %d because it exceeds limit of %d\n", len, MAX_ARG_LENGTH);
        for (;;) {}
    }

    memcpy((void*) ((uint32_t) process->physical_stack + STACK_TOP_OFFSET - offset - len), str, len);
    process->esp -= len;
    //ret = process->esp + 1;
    ret = process->esp;

    // Align at 4-bytes
    process->esp -= process->esp % 4;

    return ret;
}

void set_kernel_stack(uint32_t esp) {

    // Setting ss0 just in case
    sys_tss.ss0 = 0x10;  /* Kernel data segment */
    sys_tss.esp0 = esp;
}
