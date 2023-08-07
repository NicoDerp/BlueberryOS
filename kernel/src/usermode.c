
#include <kernel/usermode.h>
#include <kernel/file.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/logging.h>
#include <kernel/errors.h>

#include <asm-generic/errno-values.h>
#include <sys/mman.h>
#include <shadow.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>


extern __attribute__((noreturn)) void enter_usermode(uint32_t addr, uint32_t stack_ptr, regs_t regs);

void initialize_tss(tss_t* tss);


process_t processes[PROCESSES_MAX];
uint32_t currentProcessID;

user_t users[MAX_USERS];
group_t groups[MAX_GROUPS];

user_t* rootUser = &users[0];
group_t* rootPGroup = &groups[0];

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

    VERBOSE("freeProcessPagedirectory: with freeReadOnly %d\n", freeReadOnly);

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

    VERBOSE("freeProcessPagedirectory: freeing pagedirectory\n");
    kfree_frame(pd);
}

// Check if virtaddr doesn't pagefault
bool resolveProcessAddress(process_t* process, const void* virtaddr, uint32_t count, bool needrw) {

    uint32_t vaddr = (uint32_t) virtaddr;
    unsigned int pages = 0;
    while (count > 0) {

        uint32_t pti = vaddr / FRAME_4MB + (pages / 1024);
        uint32_t pi = ((vaddr / FRAME_4KB) & 0x03FF) + (pages & 1023);

        if (pti >= 0xC0000000 / FRAME_4MB)
            return false;

        if (!(process->pd[pti] & 1))
            return false;

        pagetable_t pagetable = getPagetable(process->pd[pti]);
        if (!(pagetable[pi] & 1) || (!(pagetable[pi] & PAGE_USER)) || (needrw && !(pagetable[pi] & PAGE_READWRITE)))
            return false;

        count = count >= FRAME_4KB ? count - FRAME_4KB : 0;
        pages++;
    }

    return true;
}

// Does the same but way more costly
bool resolveZeroProcessAddress(process_t* process, const void* virtaddr) {

    uint32_t vaddr = (uint32_t) virtaddr;
    unsigned int pages = 0;
    while (true) {

        uint32_t pti = vaddr / FRAME_4MB + (pages / 1024);
        uint32_t pi = ((vaddr / FRAME_4KB) & 0x03FF) + (pages & 1023);

        if (pti >= 0xC0000000 / FRAME_4MB)
            return false;

        if (!(process->pd[pti] & 1))
            return false;

        pagetable_t pagetable = getPagetable(process->pd[pti]);
        if (!(pagetable[pi] & 1) || (!(pagetable[pi] & PAGE_USER)))
            return false;

        uint32_t* page = (uint32_t*) getPageLocation(pagetable[pi]);
        for (unsigned int i = 0; i < FRAME_4KB; i++) {
            if (page[i] == '\0')
                return true;
        }

        pages++;
    }

    // Can't really get here but just in-case
    return false;
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

process_t* newProcess(file_t* file, user_t* user) {

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
    process->cwdir = user->iwdir;
    process->owner = user;

    if (!process->cwdir) {
        ERROR("Failed to find root directory for process\n");
    }

    if (!rootUser) {
        FATAL("No root user!\n");
        kabort();
    }

    if (!user) {
        FATAL("No current user!\n");
        kabort();
    }

    process->initialized = true;
    process->overwritten = false;
    process->parent = (process_t*) 0;

    strcpy(process->variables[0].key, "PATH");
    strcpy(process->variables[0].value, "/bin;/usr/bin");
    process->variables[0].active = true;

    process->stdinIndex = 0;
    process->stdinSize = MIN_STDIN_BUFFER_SIZE;
    process->stdinBuffer = (char*) kmalloc(MIN_STDIN_BUFFER_SIZE);

    /*
    strcpy(process->variables[1].key, "USER");
    strcpy(process->variables[1].value, user->name);
    process->variables[1].active = true;
    */

    return process;
}

process_t* newProcessArgs(file_t* file, char* args[], user_t* user) {

    process_t* process = newProcess(file, user);
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

    // argv must be zero-terminated
    processPush(process, 0);

    // Push string pointers
    for (int i = 0; i < argCount; i++) {
        int j = argCount - i - 1;

        VERBOSE("setProcessArgs: Pushing argv[%d] as 0x%x\n", j, strPointers[j]);

        processPush(process, strPointers[j]);

    }

    uint32_t esp = process->esp;

    // argv
    processPush(process, esp);

    // Push argc
    processPush(process, argCount);

    VERBOSE("setProcessArgs: Pushing argCount as %d\n", argCount);
}

int overwriteArgs(process_t* process, char* filename, const char** args, int* errnum) {

    VERBOSE("overwriteArgs: Overwriting process %d:%s with %s\n", process->id, process->name, filename);

    file_t* file = getFileWEnv(process, "PATH", filename);
    if (!file) {
        /* The file pathname or a script or ELF interpreter does not exist. */
        *errnum = ENOENT;
        return -1;
    }

    if (!fileAccessAllowed(process, file, P_EXECUTE)) {
        /* Execute permission is denied for the file or a script or ELF interpreter. */
        *errnum = EACCES;
        return -1;
    }

    size_t len = strlen(file->fullpath);
    if (len > PROCESS_MAX_NAME_LENGTH) {
        ERROR("Max process name reached!\n");
        /* Insufficient kernel memory was available */
        *errnum = ENOMEM;
        return -1;
    }


    // Backup
    pagedirectory_t oldPD = process->pd;
    uint32_t oldID = process->id;
    process_t* oldParent = process->parent;
    uint32_t oldIndexInParent = process->indexInParent;
    directory_t* oldCwdir = process->cwdir;
    user_t* oldOwner = process->owner;

    memset(&process->pfds, 0, sizeof(pfd_t) * MAX_FILE_DESCRIPTORS);
    memset(&process->regs, 0, sizeof(regs_t));

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

    process->id = oldID;
    process->indexInParent = oldIndexInParent;
    process->cwdir = oldCwdir;
    process->parent = oldParent;
    process->state = RUNNING;
    process->eip = process->entryPoint;
    process->esp = process->virtual_stack_top;
    process->file = file;
    process->owner = oldOwner;
    process->initialized = true;
    process->overwritten = true;

    process->stdinIndex = 0;
    memset(process->stdinBuffer, 0, process->stdinSize);

    // Keep enviromental vairables
    //strcpy(process->variables[0].key, "PATH");
    //strcpy(process->variables[0].value, "/bin;/usr/bin");
    //process->variables[0].active = true;

    // Keep parent
    //process->parent = (process_t*) 0;

    // Keep cwdir
    //process->cwdir = getDirectory("/");

    // Keep enviroment variables

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
            /* The total number of bytes in the environment (envp) and argument list (argv) is too large. */
            *errnum = E2BIG;
            for (;;) {}
        }

        // Include '\0'
        memcpy(argsCopy[i], args[i], len + 1);
        argPointers[i] = argsCopy[i];
    }

    argPointers[argCount] = NULL;

    // Add args
    //loadPageDirectory(process->pd);
    setProcessArgs(process, argPointers);

    // Free pagedirectory since that is kalloc'ed in loadELF/Binary IntoMemory
    VERBOSE("overwriteArgs: freeing pagedirectory\n");
    freeProcessPagedirectory(oldPD, oldParent == 0);

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

    if (process->parent) {
        VERBOSE("terminateProcess: Terminating process %d:%s with parent %d:%s\n", process->id, process->name, process->parent->id, process->parent->name);
    } else {
        VERBOSE("terminateProcess: Terminating process %d:%s without parent\n", process->id, process->name);
    }

    kfree(process->stdinBuffer);

    for (size_t i = 0; i < MAX_CHILDREN; i++) {

        process_t* child = process->children[i];
        if (child != 0) {

            VERBOSE("terminateProcess: Terminating child %d:%s\n", child->id, child->name);

            // Doesn't really matter since we are terminating also
            //process->children[i] = 0;

            child->initialized = false;

            VERBOSE("terminateProcess: freeing pagedirectory of child\n");

            freeProcessPagedirectory(child->pd, child->overwritten);
        }
    }

    VERBOSE("terminateProcess: freeing pagedirectory\n");
    freeProcessPagedirectory(process->pd, process->overwritten);
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
    child->overwritten = false;

    // Copy stack
    //memcpy((void*) child->physical_stack, (void*) parent->physical_stack, FRAME_4KB);

    child->pd = copy_system_pagedirectory();

    for (size_t i = 0; i < 768; i++) {

        // Check if pagetable is present
        if (parent->pd[i] & 1) {

            pagetable_t pt = getPagetable(parent->pd[i]);

            for (size_t j = 0; j < 1024; j++) {

                // Check if page is present
                if (pt[j] & 1) {

                    uint32_t pageLoc = getPageLocation(pt[j]);
                    uint32_t virtualAddr = i*FRAME_4MB + j*FRAME_4KB;

                    // Check if page is writable
                    if (pt[j] & 2) {

                        pageframe_t pageframe = kalloc_frame();

                        VERBOSE("forkProcess: copying page from 0x%x to 0x%x\n", pageLoc, pageframe);
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

    /*
    printUserPagedirectory(parent->pd);
    printUserPagedirectory(child->pd);
    */

    // Copy registers
    memcpy(&child->regs, &parent->regs, sizeof(regs_t));

    // Copy other parameters
    child->parent = parent;
    child->esp = parent->esp;
    child->eip = parent->eip;
    child->entryPoint = parent->entryPoint;
    child->cwdir = parent->cwdir;
    child->file = parent->file;
    child->owner = parent->owner;
    child->indexInParent = index;
    parent->children[index] = child;

    strcpy(child->name, parent->name);

    memcpy(&child->variables, &parent->variables, sizeof(env_variable_t)*MAX_ENVIROMENT_VARIABLES);
    memcpy(&child->pfds, &parent->pfds, sizeof(pfd_t)*MAX_FILE_DESCRIPTORS);

    child->stdinIndex = 0;
    child->stdinSize = MIN_STDIN_BUFFER_SIZE;
    child->stdinBuffer = (char*) kmalloc(MIN_STDIN_BUFFER_SIZE);

    parent->regs.eax = child->id;
    child->regs.eax = 0;
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

    // Load process's page directory                    //memcpy(pageframe+offset, data, program->filesz);
                    //memcpy(pageframe + program->offset, data, program->filesz);
                    //memcpy(pageframe + physOffset, data, program->filesz);

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

bool fileAccessAllowed(process_t* process, file_t* file, unsigned int check) {

    uint32_t mode = file->mode;

    /* Use 'owner' permissions*/
    if (file->owner->uid == process->owner->uid) {

        mode >>= 6;
    }

    /* Use 'group' permissions */
    else if (userInGroup(process->owner, file->group)) {

        mode >>= 3;

    }

    /* Use 'other' permissions */
    /*
    else {

    }
    */

    return (mode & check) > 0;
}

bool directoryAccessAllowed(process_t* process, directory_t* dir, uint32_t check) {

    uint32_t mode = dir->mode;

    /* Use 'user' permissions*/
    if (dir->owner->uid == process->owner->uid) {

        mode >>= 6;
    }

    /* Use 'group' permissions */
    else if (userInGroup(process->owner, dir->group)) {

        mode >>= 3;

    }

    /* Use 'other' permissions */
    /*
    else {

    }
    */

    return (mode & check) > 0;
}

bool userInGroup(user_t* user, group_t* group) {

    for (size_t i = 0; i < MAX_GROUP_MEMBERS; i++) {
        if (group->members[i] && group->members[i]->uid == user->uid)
            return true;
    }

    return false;
}

void addUserToGroup(user_t* user, group_t* group) {

    bool found = false;
    size_t gindex;
    for (gindex = 0; gindex < MAX_GROUP_MEMBERS; gindex++) {
        if (!group->members[gindex]) {
            found = true;
            break;
        }
    }

    if (!found) {
        ERROR("Max users for group %s reached!\n", group->name);
        return;
    }

    found = false;
    size_t uindex;
    for (uindex = 0; uindex < MAX_USER_GROUPS; uindex++) {
        if (!user->groups[uindex]) {
            found = true;
            break;
        }
    }

    if (!found) {
        FATAL("Max groups for user %s reached!\n", user->name);
        return;
    }

    group->members[gindex] = user;
    user->groups[uindex] = group;
}

group_t* createGroup(char* name) {

    bool found = false;
    size_t index;
    for (index = 0; index < MAX_GROUPS; index++) {
        if (!groups[index].active) {
            found = true;
            break;
        }
    }

    if (!found) {
        FATAL("Max groups reached!\n");
        kabort();
        return (group_t*) 0;
    }

    group_t* group = &groups[index];

    if (strlen(name) > MAX_GROUP_NAME_LENGTH) {
        FATAL("Group name exceeds max length\n");
        kabort();
        return (group_t*) 0;
    }

    strcpy(group->name, name);
    memset(group->members, 0, sizeof(user_t*)*MAX_GROUP_MEMBERS);
    group->gid = index;
    group->active = true;

    return group;
}

user_t* createUser(char* name, char* password, bool createHome, bool root) {

    bool found = false;
    size_t index;
    for (index = 0; index < MAX_USERS; index++) {
        if (!users[index].active) {
            found = true;
            break;
        }
    }

    if (!found) {
        ERROR("Max users reached!\n");
        return (user_t*) 0;
    }

    user_t* user = &users[index];
    memset(user, 0, sizeof(user));

    if (strlen(name) > MAX_USERNAME_LENGTH) {
        ERROR("Username exceeds max length\n");
        return (user_t*) 0;
    }

    if (strlen(password) > MAX_PASSWORD_LENGTH) {
        ERROR("Password exceeds max length\n");
        return (user_t*) 0;
    }

    strcpy(user->name, name);
    strcpy(user->password, password);

    addUserToGroup(user, createGroup(name));
    user->root = root;
    user->uid = index;
    user->active = true;

    file_t* program = getFile("/bin/shell");
    if (!program) {
        FATAL("Couldn't find /bin/shell as initial program for user\n");
        kabort();
    }
    user->program = program;

    if (root) {
        if (user != &users[0]) {
            FATAL("Root user is not first user!\n");
            kabort();
        }

        if (rootUser->groups[0] != &groups[0]) {
            FATAL("Root user is not first user!\n");
            kabort();
        }
    }

    if (createHome) {
        directory_t* homeDir = getDirectory("/home");
        if (!homeDir) {
            homeDir = createDirectory(&rootDir, "home", 0755, rootUser, rootUser->groups[0], NULL);

            if (!homeDir) {
                FATAL("Failed to create /home directory\n");
                kabort();
            }
        }

        user->home = getDirectoryFromParent(homeDir, name, true);
        if (user->home) {

            // Change permissions from root to user
            changeDirectoryOwner(homeDir, user, user->groups[0], true);
        }
        else {
            user->home = createDirectory(homeDir, name, 0755, user, user->groups[0], NULL);
            if (!user->home) {
                FATAL("Failed to create /home/%s directory\n", name);
                kabort();
            }
        }
        user->iwdir = user->home;

    } else {
        user->home = (directory_t*) 0;
        user->iwdir = &rootDir;
    }

    return user;
}

user_t* getUserByUID(uint32_t uid) {

    for (size_t i = 0; i < MAX_USERS; i++) {
        if (users[i].active && users[i].uid == uid)
            return &users[i];
    }

    return (user_t*) 0;
}

user_t* getUserByName(char* name) {

    for (size_t i = 0; i < MAX_USERS; i++) {
        if (users[i].active && strcmp(users[i].name, name) == 0)
            return &users[i];
    }

    return (user_t*) 0;
}

group_t* getGroupByGID(uint32_t gid) {

    for (size_t i = 0; i < MAX_GROUPS; i++) {
        if (groups[i].active && groups[i].gid == gid)
            return &groups[i];
    }

    return (group_t*) 0;
}

int getPasswdStructR(uint32_t uid, struct passwd* pwd, char* buffer, uint32_t bufsize, struct passwd** result) {

    *result = NULL;

    // TODO error value
    user_t* user = getUserByUID(uid);
    if (!user) {
        /* The user profile associated with the UID was not found. */
        return ENOENT;
    }

    if (strlen(user->name)+strlen(user->iwdir->fullpath)+strlen(user->program->fullpath)+3 > bufsize) {
        /* Insufficient storage was supplied through buffer and bufsize to contain the data to be referenced by the resulting group structure. */
        return ERANGE;
    }

    /* pw_name */
    pwd->pw_name = buffer;
    strcpy(buffer, user->name);
    size_t index = strlen(user->name) + 1;

    /* pw_uid */
    pwd->pw_uid = uid;

    /* pw_gid */
    if (user->groups[0])
        pwd->pw_gid = user->groups[0]->gid;
    else
        pwd->pw_gid = 0;

    /* pw_dir */
    pwd->pw_dir = buffer + index;
    strcpy(buffer + index, user->iwdir->fullpath);
    index += strlen(user->iwdir->fullpath) + 1;

    /* pw_shell */
    pwd->pw_shell = buffer + index;
    strcpy(buffer + index, user->program->fullpath);
    index += strlen(user->program->fullpath) + 1;

    *result = pwd;

    return 0;
}

int getGroupStructR(uint32_t gid, struct group* grp, char* buffer, size_t bufsize, struct group** result) {

    *result = NULL;

    // TODO error value
    group_t* group = getGroupByGID(gid);
    if (!group) {
        return 1;
    }

    size_t groupNameLen = strlen(group->name);
    if (groupNameLen+1 > bufsize)
        return 1;

    /* gr_name */
    grp->gr_name = buffer;
    memcpy(buffer, group->name, groupNameLen+1);
    size_t index = groupNameLen + 1;

    /* gr_gid */
    grp->gr_gid = gid;

    /* gr_mem */
    size_t oldIndex = index;

    // First write strings
    for (size_t i = 0; i < MAX_GROUP_MEMBERS; i++) {
        if (group->members[i]) {
            int memberLen = strlen(group->members[i]->name);
            if (memberLen+index+1 > bufsize)
                return 1;

            memcpy(buffer + index, group->members[i]->name, memberLen+1);
            index += memberLen + 1;
        }
    }

    //index /= sizeof(char**);
    size_t bakIndex = index;

    // Then write array of those string pointers
    for (size_t i = 0; i < MAX_GROUP_MEMBERS; i++) {
        if (group->members[i]) {
            size_t memberLen = strlen(group->members[i]->name);
            if (index+1 > bufsize)
                return 1;

            ((char**) buffer)[index++] = buffer + oldIndex;
            oldIndex += memberLen + 1;
        }
    }

    // Zero terminate array
    buffer[index] = '\0';

    grp->gr_mem = &((char**) buffer)[bakIndex];

    *result = grp;

    return 0;
}

int getSpwdStructR(char* name, struct spwd* spw, char* buffer, uint32_t bufsize, struct spwd** result) {

    *result = NULL;

    // TODO error value
    user_t* user = getUserByName(name);
    if (!user) {
        /* The user profile associated with the name was not found. */
        /* Not specified in linux man page! */
        return ENOENT;
    }

    size_t userNameLen = strlen(user->name);
    size_t userPassLen = strlen(user->password);
    if (userNameLen+userPassLen+2 > bufsize) {
        /* Supplied buffer is too small. */
        return ERANGE;
    }

    /* sp_namp */
    spw->sp_namp = buffer;
    memcpy(buffer, user->name, userNameLen+1);
    size_t index = userNameLen + 1;

    /* sp_pwdp */
    spw->sp_pwdp = buffer + index;
    memcpy(buffer + index, user->password, userPassLen);
    index += userPassLen + 1;

    spw->sp_min = 0;
    spw->sp_max = 0;
    spw->sp_warn = 0;
    spw->sp_inact = 0;
    spw->sp_expire = 0;
    spw->sp_flag = 0;

    *result = spw;

    return 0;
}

uint32_t getVirtualChunks(process_t* process, uint32_t address, uint32_t chunks, bool ret) {

    uint32_t cont = 0;
    for (size_t i = address/FRAME_4MB; i < 1024 && cont < chunks; i++) {
        if (!(process->pd[i] & 1)) {
            if (cont == 0)
                address = FRAME_4MB * i;

            cont += 1024;
            continue;
        } else {
            if (ret)
                return 0;

            cont = 0;
        }

        pagetable_t pt = getPagetable(process->pd[i]);

        size_t jstart;
        if (i == 0)
            jstart = (address / FRAME_4KB) & 0x03FF;
        else
            jstart = 0;

        for (size_t j = jstart; j < 1024 && cont < chunks; j++) {
            if (!(pt[j] & 1)) {
                if (cont == 0)
                    address = FRAME_4MB * i + FRAME_4KB * j;

                cont++;
            } else {
                if (ret)
                    return 0;

                cont = 0;
            }
        }
    }

    return address;
}

int mmapProcess(process_t* process, uint32_t address, uint32_t length, int prot, int flags, int fd, uint32_t offset, int* errnum) {

    // Not supported
    if (prot & PROT_EXEC || prot == PROT_NONE || fd != 0 || offset != 0 || flags & MAP_SHARED) {
        *errnum = EINVAL;
        return -1;
    }

    // Find virtual space enough for 'chunks' chunks
    uint32_t chunks = ((FRAME_SIZE - (length & (FRAME_SIZE-1)) + length)) >> 12;

    VERBOSE("mmapProcess: finding space for %d chunks\n", chunks);

    if (address == 0) {

        // Find on our own
        address = getVirtualChunks(process, MMAP_START_ADDRESS, chunks, false);
    } else {

        // First try the hint
        address = getVirtualChunks(process, address, chunks, true);

        // If that doesn't work try on our own
        if (address == 0)
            address = getVirtualChunks(process, MMAP_START_ADDRESS, chunks, false);
    }

    VERBOSE("mmapProcess: Got address 0x%x\n", address);

    if (address == 0) {
        /* No memory is available. */
        *errnum = ENOMEM;
        return -1;
    }

    for (size_t i = 0; i < chunks; i++) {
        pageframe_t frame = kalloc_frame();
        uint32_t virtual = address + i * FRAME_SIZE;
        map_page_wflags_pd(process->pd, v_to_p((uint32_t) frame), virtual, PAGE_MMAPPED | PAGE_USER | PAGE_READWRITE | PAGE_PRESENT);
    }

    return address;
}

int munmapProcess(process_t* process, uint32_t address, uint32_t length, int* errnum) {

    // If address or length is 0, or if adress isn't page-aligned
    if ((address == 0) || (length == 0) || (address & (FRAME_SIZE-1))) {
        *errnum = EINVAL;
        return -1;
    }

    uint32_t chunks = (length + FRAME_SIZE - 1) / FRAME_SIZE;

    uint32_t cleared = 0;
    for (uint32_t i = address/FRAME_4MB; (i < 1024) && (cleared < chunks); i++) {

        if (!(process->pd[i] & PAGE_PRESENT))
            continue;

        pagetable_t pt = getPagetable(process->pd[i]);

        size_t jstart;
        if (i == address/FRAME_4MB)
            jstart = (address / FRAME_4KB) & 0x03FF;
        else {
            jstart = 0;
        }

        for (uint32_t j = jstart; (j < 1024) && (cleared < chunks); j++) {

            if (!(pt[j] & PAGE_PRESENT) || !(pt[j] & PAGE_MMAPPED))
                continue;

            uint32_t page = getPageLocation(pt[j]);
            kfree_frame((void*) page);

            VERBOSE("munmap: Freeing 0x%x\n", i*FRAME_4MB + j*FRAME_4KB);

            // Unmapping
            pt[j] = 0;

            cleared++;
        }
    }

    return 0;
}

env_variable_t* getEnvVariable(process_t* process, const char* key) {

    for (size_t i = 0; i < MAX_ENVIROMENT_VARIABLES; i++) {

        env_variable_t* var = &process->variables[i];

        if (var->active && strcmp(var->key, key) == 0)
            return var;
    }

    return (env_variable_t*) 0;
}

int setEnvVariable(process_t* process, const char* key, const char* value, bool overwrite) {

    size_t len;

    len = strlen(key);
    if (len > MAX_VARIABLE_KEY_LENGTH) {
        ERROR("setEnvVariable: Key is over max length\n");
        return -1;
    }

    len = strlen(value);
    if (len > MAX_VARIABLE_VALUE_LENGTH) {
        ERROR("setEnvVariable: Value is over max length\n");
        return -1;
    }

    size_t index = 99;
    bool found = false;
    for (size_t i = 0; i < MAX_ENVIROMENT_VARIABLES; i++) {

        env_variable_t* var = &process->variables[i];

        if (var->active) {
            if (overwrite && strcmp(var->key, key) == 0) {
                strcpy(var->value, value);
                return 0;
            }
        } else if (!found) {
            index = i;
            found = true;
        }
    }

    if (!found) {
        FATAL("Max enviroment variables reached for process %d:%s\n", process->id, process->name);
        kabort();
        return -1;
    }

    if (index == 99) {
        FATAL("setEnvVariable: huuh 99??\n");
        kabort();
        return -1;
    }

    env_variable_t* var = &process->variables[index];

    strcpy(var->key, key);
    strcpy(var->value, value);
    var->active = true;

    return 0;
}

int unsetEnvVariable(process_t* process, const char* key) {

    for (size_t i = 0; i < MAX_ENVIROMENT_VARIABLES; i++) {

        env_variable_t* var = &process->variables[i];

        if (var->active && strcmp(var->key, key) == 0) {
            var->active = false;
            return 0;
        }
    }

    // If it is not found then it is still sucess
    return 0;
}

file_t* getFileWEnv(process_t* process, char* env, char* path) {

    file_t* file;

    file = getFileFrom(process->cwdir, path, true);
    if (file)
        return file;

    env_variable_t* var = getEnvVariable(process, env);

    if (!var) {
        ERROR("Process %d:%s doesn't have enviroment variable %s\n", process->id, process->name, env);
        return (file_t*) 0;
    }

    char* ptr;
    char* last = var->value;
    bool done = false;
    while (!done) {

        ptr = strchr(last, ';');
        if (ptr == NULL) {
            ptr = var->value + strlen(var->value);
            done = true;
        }

        uint32_t len = ptr - last;
        char str[len+1];
        memcpy(str, last, len);
        str[len] = '\0';

        VERBOSE("getFileWEnv: Trying path '%s'\n", str);

        directory_t* dir = getDirectory(str);

        if (dir) {
            file = getFileFrom(dir, path, true);
            
            if (file)
                return file;
        }

        if (done)
            break;

        last = ptr + 1;
    }

    return (file_t*) 0;
}

int openProcessFile(process_t* process, char* pathname, uint32_t flags, uint32_t permissions, int* errnum) {

    if ((!(flags & O_RDONLY) && !(flags & O_WRONLY)) || ((flags & O_RDONLY) && (flags & O_TRUNC))) {

        ERROR("Flags are incorrect\n");
        *errnum = EINVAL;
        return -1;
    }

    if ((flags & O_WRONLY || flags & O_TRUNC) && (flags & O_DIRECTORY)) {
        *errnum = EISDIR;
        return -1;
    }

    size_t index;
    bool found = false;
    for (index = 0; index < MAX_FILE_DESCRIPTORS; index++) {

        if (!process->pfds[index].active) {
            found = true;
            break;
        }
    }

    if (!found) {
        ERROR("process %d:%s has reached max pfds\n", process->id, process->name);
        *errnum = ENOMEM;
        return -1;
    }

    pfd_t* pfd = &process->pfds[index];

    // Doesn't use PATH
    if (flags & O_DIRECTORY) {
        directory_t* dir = getDirectoryFrom(process->cwdir, pathname, true);

        if (!dir) {
            if (!(flags & O_CREAT)) {
                *errnum = ENOENT;
                return -1;
            }

            // TODO support creating directories also and not just end file
            size_t slash;
            directory_t* parent = findParent(process->cwdir, pathname, &slash, false);

            if (!parent) {
                // TODO switch
                *errnum = ENOENT;
                return -1;
            }

            if (!directoryAccessAllowed(process, parent, P_WRITE)) {
                *errnum = EACCES;
                return -1;
            }

            group_t* group;
            if (process->owner->groups[0])
                group = process->owner->groups[0];
            else
                group = parent->group;

            dir = createDirectory(parent, pathname+slash, permissions, process->owner, group, errnum);
            if (!dir)
                return -1;
        }

        if (!directoryAccessAllowed(process, dir, P_READ)) {
            *errnum = EACCES;
            return -1;
        }

        pfd->pointer = (uint32_t) dir;
    } else {
        file_t* file = getFileFrom(process->cwdir, pathname, true);

        if (!file) {

            if (!(flags & O_CREAT)) {
                *errnum = ENOENT;
                return -1;
            }

            // TODO support creating directories also and not just end file
            size_t slash;
            directory_t* parent = findParent(process->cwdir, pathname, &slash, false);

            if (!parent) {
                // TODO switch
                *errnum = ENOENT;
                return -1;
            }

            if (!directoryAccessAllowed(process, parent, P_WRITE)) {
                *errnum = EACCES;
                return -1;
            }

            group_t* group;
            if (process->owner->groups[0])
                group = process->owner->groups[0];
            else
                group = parent->group;

            file = createFile(parent, pathname+slash, permissions, process->owner, group, errnum);
            if (!file)
                return -1;
        }

        if (!fileAccessAllowed(process, file, P_READ)) {
            *errnum = EACCES;
            return -1;
        }

        pfd->pointer = (uint32_t) file;

        if (flags & O_TRUNC) {
            if (!fileAccessAllowed(process, file, P_WRITE)) {
                *errnum = EACCES;
                return -1;
            }

            memset(file->content, 0, file->size);
            file->size = 0;
        }
    }

    pfd->position = 0;
    pfd->fd = index + 3; // Because of stdin, out and err
    pfd->flags = flags;
    pfd->active = true;

    return pfd->fd;
}

int readProcessFd(process_t* process, char* buf, size_t count, unsigned int fd) {

    pfd_t* pfd = getProcessPfd(process, fd);
    if (!pfd)
        return -1;

    if (!pfd->active)
        return -1;

    if (pfd->flags & O_DIRECTORY)
        return -1;

    if (!(pfd->flags & O_RDONLY))
        return -1;

    file_t* file = (file_t*) pfd->pointer;

    // EOF
    if (pfd->position >= file->size)
        return 0;

    // We read a maximum of bytes that the file has minus where we are
    if (file->size - pfd->position < count)
        count = file->size - pfd->position;

    memcpy(buf, file->content + pfd->position, count);
    pfd->position += count;

    return count;
}

int writeProcessFd(process_t* process, char* buf, size_t count, unsigned int fd, int* errnum) {

    pfd_t* pfd = getProcessPfd(process, fd);
    if (!pfd) {
        /* fd is not a valid file descriptor or is not open for writing. */
        *errnum = EBADF;
        return -1;
    }

    if (!pfd->active) {
        /* fd is not a valid file descriptor or is not open for writing. */
        *errnum = EBADF;
        return -1;
    }

    if (pfd->flags & O_DIRECTORY) {
        /* fd is not a valid file descriptor or is not open for writing. */
        *errnum = EBADF;
        return -1;
    }

    if (!(pfd->flags & O_WRONLY)) {
        /* fd is not a valid file descriptor or is not open for writing. */
        *errnum = EBADF;
        return -1;
    }

    file_t* file = (file_t*) pfd->pointer;

    if (pfd->position + count > file->size)
        file->size = pfd->position + count;

    // Reallocate
    if (pfd->position + count >= file->frames*FRAME_4KB) {
        unsigned int size = pfd->position + count;
        unsigned int frames = (file->size+FRAME_4KB-1)/FRAME_4KB;

        pageframe_t content = kalloc_frames(frames);

        if (file->content != NULL) {
            memcpy(content, file->content, file->size);
            kfree_frames(file->content, file->frames);
        }

        file->content = content;
        file->size = size;
        file->frames = frames;
    }

    memcpy(file->content + pfd->position, buf, count);
    pfd->position += count;

    return count;
}

int closeProcessFd(process_t* process, unsigned int fd) {

    pfd_t* pfd = getProcessPfd(process, fd);
    if (!pfd)
        return -1;

    if (!pfd->active)
        return -1;

    process->pfds[fd].active = false;

    return 0;
}

pfd_t* getProcessPfd(process_t* process, unsigned int fd) {

    // Because of stdin, out and err
    fd -= 3;
    if (fd >= MAX_FILE_DESCRIPTORS) {
        ERROR("fd is outside range\n");
        return (pfd_t*) 0;
    }

    return &process->pfds[fd];
}

int getDirectoryEntries(process_t* process, int fd, char* buf, size_t nbytes, uint32_t* basep) {

    pfd_t* pfd = getProcessPfd(process, fd);
    if (!pfd)
        return -1;

    if (!(pfd->flags & O_DIRECTORY))
        return -1;

    directory_t* dir = (directory_t*) pfd->pointer;

    size_t startIndex;
    if (*basep == 0)
        startIndex = 0;
    else
        startIndex = *basep / sizeof(struct dirent);

    uint32_t bytesRead = 0;
    for (size_t i = 0; i < nbytes/sizeof(struct dirent); i++) {

        struct dirent* entry = &((struct dirent*) buf)[i];

        if (startIndex + i < dir->directoryCount) {

            directory_t* d = dir->directories[startIndex + i];

            entry->d_type = DT_DIR;
            strncpy(entry->d_name, d->name, 255);
            entry->d_name[255] = '\0';
        }

        else if (startIndex - dir->directoryCount + i < dir->fileCount) {

            file_t* f = dir->files[startIndex - dir->directoryCount + i];

            entry->d_type = DT_REG;
            strncpy(entry->d_name, f->name, 255);
            entry->d_name[255] = '\0';
        }

        // No more to read
        else {
            break;
        }

        entry->d_ino = 0;
        entry->d_off = 0;
        entry->d_reclen = sizeof(struct dirent);

        bytesRead += sizeof(struct dirent);
    }

    *basep += bytesRead;

    return bytesRead;
}

int statPath(process_t* process, char* path, struct stat* buf, bool redirectSymbolic, int* errnum) {

    file_t* file = getFileFrom(process->cwdir, path, redirectSymbolic);
    if (!file) {
        directory_t* dir = getDirectoryFrom(process->cwdir, path, redirectSymbolic);
        if (!dir) {
            *errnum = ENOENT;
            return -1;
        }

        if (!dir->owner) {
            FATAL("Directory %s at %s doesn't have owner!\n", dir->name, process->cwdir->name);
            kabort();
        }

        if (!dir->group) {
            FATAL("Directory %s at %s doesn't have group!\n", dir->name, process->cwdir->name);
            kabort();
        }

        if (!directoryAccessAllowed(process, dir, P_READ)) {
            *errnum = EACCES;
            return -1;
        }

        memset(buf, 0, sizeof(struct stat));
        buf->st_mode = dir->mode | S_IFDIR;
        buf->st_blksize = FRAME_4KB;

        // TODO if symbolic then length of path it points to
        buf->st_size = sizeof(directory_t);

        buf->st_uid = dir->owner->uid;
        buf->st_gid = dir->group->gid;

        return 0;
    }

    if (!file->owner) {
        FATAL("File doesn't have owner!\n");
        kabort();
    }

    if (!file->group) {
        FATAL("File doesn't have group!\n");
        kabort();
    }

    if (!fileAccessAllowed(process, file, P_READ)) {
        *errnum = EACCES;
        return -1;
    }

    memset(buf, 0, sizeof(struct stat));
    buf->st_mode = file->mode | S_IFREG;
    buf->st_blksize = FRAME_4KB;

    // TODO if symbolic then length of path it points to
    buf->st_size = file->size;

    buf->st_uid = file->owner->uid;
    buf->st_gid = file->group->gid;

    return 0;
}


void handleWaitpid(process_t* process) {

    for (size_t i = 0; i < MAX_CHILDREN; i++) {

        if (process->children[i] != 0 && process->children[i]->state == ZOMBIE) {
            VERBOSE("handleWaitpid: Terminating zombie child\n");

            // TODO status. Shouldn't be passed to terminateProcess
            terminateProcess(process->children[i], 0);

            return;
        }

    }

    // If no children are zombie then we wait
    process->state = BLOCKED_WAITPID;
}

void handleWaitpidBlock(process_t* process) {

    // No parent that could wait for this process
    if (!process->parent) {
        terminateProcess(process, 0);
        return;
    }

    // Parent is waiting
    if (process->parent->state == BLOCKED_WAITPID) {
        process->parent->state = RUNNING;

        // Sucess
        process->parent->regs.eax = process->id;

        int* status = (int*) process->parent->blocked_regs.ebx;
        if (status != NULL) {
            VERBOSE("handleWaitpidBlock: Satus from waitpid used but is unimplemented\n");
            loadPageDirectory(process->parent->pd);
            *status = 0; // TODO unimplemented WIFEXITED n shi
        }

        terminateProcess(process, 0);
    }

    // Parent is not waiting (yet (probably))
    else {

        // Now child process is in zombie state waiting to be 'wait()'ed for
        process->state = ZOMBIE;
    }
}

void handleKeyboardBlock(char c) {

    if (!processes[currentProcessID].initialized)
        return;

    for (size_t i = 0; i < PROCESSES_MAX; i++) {
        process_t* process = &processes[i];

        if (!process->initialized)
            continue;

        // If process is waiting for input
        if (process->state == BLOCKED_KEYBOARD) {

            // Quick fix
            loadPageDirectory(process->pd);

            char* buf = (char*) process->blocked_regs.ecx;

            buf[process->regs.eax] = c;   // Write to buffer
            process->blocked_regs.edx--;  // Count requested
            process->regs.eax++;          // Return value: bytes read
            
            // If counter is zero, then unblock
            if (process->blocked_regs.edx == 0)
                process->state = RUNNING;
        }

        // Then if the process isn't blocked by something else then write to stdin buffer
        // Don't bother saving for loop process
        else if (process->state == RUNNING && process->id != 0) {

            // If we reached the max then we setback STDIN_SETBACK chars
            if (process->stdinSize >= MAX_STDIN_BUFFER_SIZE) {

                memmove(process->stdinBuffer, process->stdinBuffer + MAX_STDIN_BUFFER_SIZE - STDIN_SETBACK, STDIN_SETBACK);
                continue;
            }

            if (process->stdinIndex >= process->stdinSize) {
                process->stdinSize *= 2;
                process->stdinBuffer = krealloc(process->stdinBuffer, process->stdinSize);
            }

            process->stdinBuffer[process->stdinIndex++] = c;
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
