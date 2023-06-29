
#include <kernel/usermode.h>
#include <kernel/file.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/logging.h>

#include <string.h>
#include <stdio.h>

#include <kernel/memory.h>


extern __attribute__((noreturn)) void enter_usermode(uint32_t addr, uint32_t stack_ptr, regs_t regs);

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

void freeProcessPagedirectory(pagedirectory_t pd, bool freeReadOnly) {

    VERBOSE("freProcessPagedirectory: with freeReadOnly %d\n", freeReadOnly);

    for (size_t i = 0; i < 768; i++) {
        if (pd[i] & 1) {
            VERBOSE("freeProcessPagedirectory: freeing pagetable %d\n", i);
            pagetable_t pt = getPagetable(pd[i]);

            // TODO this only works now because the only things
            //  I map is the ELF.
            // In future I need to keep track of which pages to free.
            // Idea: Pages have bits 9-11 available.
            // Perfect for storing a flag if we need to free this page!!
            for (size_t j = 0; j < 1024; j++) {
                if (pt[j] & 1) {

                    // Readwrite
                    if (pt[j] & 2) {
                        uint32_t page = getPageLocation(pt[j]);

                        VERBOSE("freeProcessPageDirectory: freeing readwrite page %d pointing to 0x%x\n", j, page);
                        kfree_frame((void*) page);
                    }

                    // Readonly
                    else if (freeReadOnly) {
                        uint32_t page = getPageLocation(pt[j]);

                        VERBOSE("freeProcessPageDirectory: freeing readonly page %d pointing to 0x%x\n", j, page);
                        kfree_frame((void*) page);
                    }
                }
            }

            kfree_frame((void*) pt);
        }
    }

    VERBOSE("freProcessPagedirectory: freeing pagedirectory\n");
    kfree_frame(pd);
}

process_t* findNextProcess(void) {

    //VERBOSE("findNextProcess\n");

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
            ERROR("No current processes?? Idk what to do now\n");
            for (;;) {}
            return (process_t*) 0;
        }

        process = &processes[currentProcessID];

        if (process->state != RUNNING) {
            ERROR("Only one process to run and that is blocked\n");
        }
    }

    //VERBOSE("findNextProcess: Found process %d:%s\n", process->id, process->name);

    return process;
}

process_t* getCurrentProcess(void) {
    return &processes[currentProcessID];
}

process_t* newProcess(file_t* file) {

    process_t* process;
    bool found = false;
    uint32_t index;
    for (index = 0; index < PROCESSES_MAX; index++) {
        if (!processes[index].initialized) {
            found = true;
            process = &processes[index];
            memset(process, 0, sizeof(process_t));
            break;
        }
    }

    if (!found) {
        ERROR("Max processes reached!\n");
        for (;;) {}
        return (process_t*) 0;
    }

    VERBOSE("newProcess: Creating new process with id %d\n", index);

    if (index >= PROCESSES_MAX) {
        ERROR("Can't create process with pid outside maximum limit\n");
        for (;;) {}
    }

    process->id = index;

    size_t len = strlen(file->fullpath);
    if (len > PROCESS_MAX_NAME_LENGTH) {
        ERROR("Max process name reached!\n");
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

    VERBOSE("newProcess: Physical stack at 0x%x. Virtual at 0x%x\n", process->physical_stack, process->virtual_stack);
    map_page_pd(process->pd, v_to_p((uint32_t) process->physical_stack), process->virtual_stack, true, false);

    process->state = RUNNING;
    process->eip = process->entryPoint;
    process->esp = process->virtual_stack_top;
    process->file = file;
    process->cwdir = getDirectory("/");

    if (!process->cwdir) {
        ERROR("Failed to find root directory for process\n");
    }

    process->initialized = true;
    process->parent = (process_t*) 0;

    return process;
}

process_t* newProcessArgs(file_t* file, char* args[]) {

    process_t* process = newProcess(file);
    setProcessArgs(process, args);

    return process;
}

void setProcessArgs(process_t* process, char* args[]) {

    int argCount;
    for (argCount = 0; args[argCount] != 0; argCount++) {}

    VERBOSE("setProcessArgs: argCount %d\n", argCount);

    if (argCount >= MAX_ARGS) {
        ERROR("Can't create process with arg count %d because it exceeds limit of %d\n", argCount, MAX_ARGS);
        for (;;) {}
    }

    uint32_t strPointers[argCount];

    // Push strings
    for (int i = 0; i < argCount; i++) {
        VERBOSE("setProcessArgs: Pushing str[%d] as '%s'\n", i, args[i]);

        strPointers[i] = processPushStr(process, args[i]);
    }

    // Push string pointers
    for (int i = 0; i < argCount; i++) {
        int j = argCount - i - 1;

        VERBOSE("setProcessArgs: Pushing argv[%d] as 0x%x\n", j, strPointers[j]);

        processPush(process, strPointers[j]);

    }

    uint32_t esp = process->esp;

    // argv must be zero-terminated
    processPush(process, 0);

    // argv
    processPush(process, esp);

    // Push argc
    processPush(process, argCount);

    VERBOSE("setProcessArgs: Pushing argCount as %d\n", argCount);
}

int overwriteArgs(process_t* process, char* filename, const char** args) {

    VERBOSE("overwriteArgs: Overwriting process %d:%s with %s\n", process->id, process->name, filename);

    file_t* file = getFileFrom(process->cwdir, filename);
    if (!file) {
        return -1;
    }

    // Backup
    pagedirectory_t oldPD = process->pd;
    pageframe_t oldStack = process->physical_stack;
    bool freeReadOnly = process->parent == 0;

    memset(process->children, 0, sizeof(process_t*) * MAX_CHILDREN);
    memset(&process->regs, 0, sizeof(regs_t));

    if (process->id >= PROCESSES_MAX) {
        ERROR("Can't create process with pid outside maximum limit\n");
        return -1;
    }

    size_t len = strlen(file->fullpath);
    if (len > PROCESS_MAX_NAME_LENGTH) {
        ERROR("Max process name reached!\n");
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

    process->physical_stack = kalloc_frame();

    memset((void*) process->physical_stack, 0, FRAME_4KB);

    process->virtual_stack = 4*FRAME_4KB; // Place stack at some place 4KB
    process->virtual_stack_top = process->virtual_stack + STACK_TOP_OFFSET;

    VERBOSE("overwriteArgs: Physical stack at 0x%x. Virtual at 0x%x\n", process->physical_stack, process->virtual_stack);
    map_page_pd(process->pd, v_to_p((uint32_t) process->physical_stack), process->virtual_stack, true, false);

    process->state = RUNNING;
    process->eip = process->entryPoint;
    process->esp = process->virtual_stack_top;
    process->file = file;
    process->initialized = true;

    // Keep parent
    //process->parent = (process_t*) 0;

    // Keep cwdir
    //process->cwdir = getDirectory("/");

    uint32_t argCount;
    for (argCount = 0; args[argCount] != 0; argCount++) {}
    VERBOSE("overwriteArgs: argCount: %d\n", argCount);

    // Hack to not have to lookup in pagedirectory
    //char argsCopy[MAX_ARG_LENGTH+1][argCount];
    char argsCopy[argCount][MAX_ARG_LENGTH+1];
    char* argPointers[argCount+1];

    for (size_t i = 0; i < argCount; i++) {
        size_t len = strlen(args[i]);
        if (len > MAX_ARG_LENGTH) {
            ERROR("Argument is over max length\n");
            for (;;) {}
        }

        // Include '\0'
        memcpy(argsCopy[i], args[i], len + 1);
        argPointers[i] = argsCopy[i];
    }

    argPointers[argCount] = NULL;

    // Add args
    loadPageDirectory(process->pd);
    setProcessArgs(process, argPointers);

    // Free pagedirectory since that is kalloc'ed in loadELF/Binary IntoMemory
    VERBOSE("overwriteArgs: freeing pagedirectory\n");
    freeProcessPagedirectory(oldPD, freeReadOnly);

    if (oldStack) {
        VERBOSE("overwriteArgs: freeing old stack\n");
        kfree_frame(oldStack);
    }

    uint32_t count = get_used_memory();
    printf("Used memory: %d\n", count);

    return 0;
}

void terminateProcess(process_t* process, int status) {

    (void) status;

    process->initialized = false;

    if (process->parent) {
        process->parent->children[process->indexInParent] = 0;
    }

    // Don't need to clear process because it will get initialized
    // memset(process, 0, sizeof(process_t))

    VERBOSE("terminateProcess: Terminating process %d:%s\n", process->id, process->name);

    // TODO free malloc stuff

    VERBOSE("terminateProcess: freeing pagedirectory\n");
    freeProcessPagedirectory(process->pd, process->parent == 0);

    if (process->physical_stack) {
        VERBOSE("terminateProcess: freeing stack\n");
        kfree_frame(process->physical_stack);
    }

    uint32_t count = get_used_memory();
    printf("Used memory: %d\n", count);
}

void runProcess(process_t* process) {

    currentProcessID = process->id;

    // Load process's page directory
    loadPageDirectory(process->pd);

    // Reset PIT count
    pit_set_count(PROCESS_TIME);

    VERBOSE("runProcess: Entering process %d:%s at 0x%x with esp 0x%x\n", process->id, process->name, process->eip, process->esp);

    // Enter usermode
    enter_usermode(process->eip, process->esp, process->regs);

    __builtin_unreachable();
}

void forkProcess(process_t* parent) {

    VERBOSE("forkProcess: Forking process %d:%s\n", parent->id, parent->name);

    bool found = false;
    size_t index;
    for (index = 0; index < MAX_CHILDREN; index++) {
        if (!parent->children[index]) {
            found = true;
            break;
        }
    }

    if (!found) {
        ERROR("Max children reached for process %d\n", parent->id);

        // Set to indicate error
        parent->regs.eax = -1;
        return;
    }


    process_t* child;
    found = false;
    for (index = 0; index < PROCESSES_MAX; index++) {
        if (!processes[index].initialized) {
            found = true;
            child = &processes[index];
            memset(child, 0, sizeof(process_t));
            break;
        }
    }

    if (!found) {
        ERROR("Max processes reached!\n");
        for (;;) {}
        return;
    }

    VERBOSE("forkProcess: Creating new process with id %d\n", index);

    if (index >= PROCESSES_MAX) {
        ERROR("Can't create process with pid outside maximum limit\n");
        for (;;) {}
        return;
    }

    // Set parameters
    child->id = index;
    child->state = RUNNING;
    //child->physical_stack
    child->initialized = true;

    // Copy stack
    //memcpy((void*) child->physical_stack, (void*) parent->physical_stack, FRAME_4KB);

    child->pd = copy_system_pagedirectory();

    for (size_t i = 0; i < 768; i++) {

        // Check if pagetable is present
        if (parent->pd[i] & 1) {

            pagetable_t pt = getPagetable(parent->pd[i]);

            // Check if pagetable is writable
            //if (pt[i] & 2)

            for (size_t j = 0; j < 1024; j++) {

                // Check if page is present
                if (pt[j] & 1) {

                    uint32_t page = pt[j];
                    uint32_t pageLoc = getPageLocation(page);
                    uint32_t virtualAddr = i*FRAME_4MB + j*FRAME_4KB;

                    // Check if page is writable
                    if (page & 2) {

                        VERBOSE("forkProcess: copying page\n");
                        pageframe_t pageframe = kalloc_frame();

                        memcpy(pageframe, (void*) pageLoc, FRAME_4KB);

                        VERBOSE("forkProcess: mapping 0x%x(p) to 0x%x(v)\n", v_to_p((uint32_t) pageframe), virtualAddr);
                        // Readwrite and user for both page and table
                        map_page_wtable_pd(child->pd, v_to_p((uint32_t) pageframe), virtualAddr, true, false, true, false);
                    }
                    
                    else {

                        VERBOSE("forkProcess: mapping to existing page\n");
                        VERBOSE("forkProcess: mapping 0x%x(p) to 0x%x(v)\n", v_to_p(pageLoc), virtualAddr);

                        map_page_wtable_pd(child->pd, v_to_p(pageLoc), virtualAddr, false, false, true, false);
                    }
                }
            }
        }
    }

    // Copy registers
    memcpy(&child->regs, &parent->regs, sizeof(regs_t));

    // Copy other parameters
    child->parent = parent;
    child->esp = parent->esp;
    child->eip = parent->eip;
    child->entryPoint = parent->entryPoint;
    child->cwdir = parent->cwdir;
    child->indexInParent = index;
    parent->children[index] = child;

    strcpy(child->name, parent->name);
    child->file = parent->file;

    parent->regs.eax = child->id;
    child->regs.eax = 0;

    // TODO in future also copy pagedirectory for things like malloc
}

void switchProcess(void) {

    //printf("Next process\n");
    // Simple round robin

    process_t* process = findNextProcess();
    /*
    if (currentProcessID != process->id)
        printf("Found next process: %d:%s\n", process->id, process->name);
    */
    currentProcessID = process->id;

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

    //VERBOSE("switchProcess: Entering process %d:%s at 0x%x with esp 0x%x\n", process->id, process->name, process->eip, process->esp);

    // Enter usermode
    enter_usermode(process->eip, process->esp, process->regs);

    __builtin_unreachable();
}

void runCurrentProcess(void) {

    process_t* process = &processes[currentProcessID];

    if (!process->initialized) {
        ERROR("Kernel tried to execute non-initialized currently running process\n");
        for (;;) {}
    }

    // Load process's page directory
    loadPageDirectory(process->pd);

    //VERBOSE("runCurrentProcess: Entering process %d:%s at 0x%x\n", process->id, process->name, process->eip);

    // Enter usermode
    enter_usermode(process->eip, process->esp, process->regs);

    __builtin_unreachable();
}

void handleWaitpidBlock(process_t* process) {

    if (!(process->parent && process->parent->state == BLOCKED_WAITPID)) {
        return;
    }

    process->parent->state = RUNNING;

    // Sucess
    process->parent->regs.eax = process->id;

    int* status = (int*) process->parent->blocked_regs.ebx;
    if (status != NULL) {
        VERBOSE("handleWaitpidBlock: Satus from waitpid used but is unimplemented\n");
        loadPageDirectory(process->parent->pd);
        *status = 0; // TODO unimplemented WIFEXITED n shi
    }
}

void handleKeyboardBlock(char c) {

    if (!processes[currentProcessID].initialized)
        return;

    for (size_t i = 0; i < PROCESSES_MAX; i++) {
        process_t* process = &processes[i];

        if (process->initialized && process->state == BLOCKED_KEYBOARD) {

            // Quick fix
            loadPageDirectory(process->pd);

            char* buf = (char*) process->blocked_regs.ecx;

            buf[process->regs.eax] = c;   // Write to buffer
            process->blocked_regs.edx--;  // Count requested
            process->regs.eax++;          // Return value: bytes read

            // If counter is zero, then unblock
            if (process->blocked_regs.edx == 0) {
                process->state = RUNNING;
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
        ERROR("Can't push arg with length %d because it exceeds limit of %d\n", len, MAX_ARG_LENGTH);
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
