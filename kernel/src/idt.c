
#include <kernel/logging.h>
#include <kernel/idt.h>
#include <kernel/io.h>
#include <kernel/tty.h>
#include <kernel/usermode.h>

#include <unistd.h>
#include <sys/syscall.h>

#include <string.h>
#include <stdbool.h>
#include <stdio.h>

// Function prototypes
void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags);
void PIC_remap(int offset1, int offset2);
void PIC_sendEOI(unsigned char irq);
extern void load_idt(idtr_t);


// Create an array of IDT entries; aligned for performance
__attribute__((aligned(0x10))) static idt_entry_t idt[IDT_USED_DESCRIPTORS];

// Create an IDTR
static idtr_t idtr;

// Table of all exceptions. Defined in idt.asm
extern void* isr_stub_table[];

static bool maskBackup;
static bool shiftDown = false;
static bool capslock = false;

//static bool vectors[32];

size_t tickCounter = 0;

const char* format_interrupt(uint8_t id) {
    if      (id == INT_INVALID_OPCODE)     { return "INVALID_OPCODE";     }
    else if (id == INT_DOUBLE_FAULT)       { return "DOUBLE_FAULT";       }
    else if (id == INT_GENERAL_PROTECTION) { return "GENERAL_PROTECTION"; }
    else if (id == INT_PAGE_FAULT)         { return "PAGE_FAULT";         }
    else if (id == INT_TIMER)              { return "TIMER";              }
    else if (id == INT_KEYBOARD)           { return "KEYBOARD";           }
    else if (id == INT_MOUSE)              { return "MOUSE";              }
    else if (id == INT_SYSCALL)            { return "SYSCALL";            }
    else                                   { return "NOT IMPLEMENTED";    }
}

typedef struct {
    unsigned int num1; // arg1
    unsigned int num2; // arg2
} __attribute__((packed)) test_struct_t;

void saveRegisters(process_t* process, stack_state_t* stack_state, interrupt_frame_t* frame, unsigned int esp) {

    process->regs.eax = stack_state->eax;
    process->regs.ebx = stack_state->ebx;
    process->regs.ecx = stack_state->ecx;
    process->regs.edx = stack_state->edx;
    process->regs.ebp = stack_state->ebp;
    process->regs.edi = stack_state->edi;
    process->regs.esi = stack_state->esi;
    process->eip = frame->eip;
    process->esp = esp;
}

//void syscall_handler(stack_state_t stack_state, test_struct_t test_struct, unsigned int interrupt_id, interrupt_frame_t frame, unsigned int esp, unsigned int ss) {
void syscall_handler(test_struct_t test_struct, unsigned int interrupt_id, stack_state_t stack_state, interrupt_frame_t frame, unsigned int esp, unsigned int ss) {
    //printf("Syscall!\n");
    (void)stack_state;
    (void)test_struct;
    (void)frame;
    (void)interrupt_id;
    (void)esp;
    (void)ss;

    /*
    printf("edi: '0x%x'\n", stack_state.edi);
    printf("esi: '0x%x'\n", stack_state.esi);
    printf("ebp: '0x%x'\n", stack_state.ebp);
    printf("esp: '0x%x'\n", stack_state.esp);
    printf("ebx: '0x%x'\n", stack_state.ebx);
    printf("edx: '0x%x'\n", stack_state.edx);
    printf("ecx: '0x%x'\n", stack_state.ecx);
    printf("eax: '0x%x'\n", stack_state.eax);

    extern tss_t sys_tss;
    printf("esp0: 0x%x\n", sys_tss.esp0);

    printf("esp: 0x%x\n", esp);
    printf("ss: 0x%x\n", ss);
    */

    /*
    {
        process_t* p = getCurrentProcess();
        printf("\nSyscall %d from %d %s\n", stack_state.eax, p->id, p->name);
    }
    */

    VERBOSE("Got syscall %d\n", stack_state.eax);

    switch (stack_state.eax) {
        case (SYS_exit):
            {
                if (!(frame.cs & 0x3)) {
                    printf("[INFO] Kernel called exit syscall?\n");
                    for (;;) {}
                }

                //printf("Exit!\n");

                int status = stack_state.ebx;

                //printf("Status: '%d'\n", status);

                process_t* process = getCurrentProcess();

                handleWaitpidBlock(process);

                printf("process called exit: %d %s\n", process->id, process->name);
                terminateProcess(process, status);

                // Switch to next process
                switchProcess();
            }

            break;

        case (SYS_write):
            {
                unsigned int fd = stack_state.ebx;
                unsigned int buf = stack_state.ecx;
                unsigned int count = stack_state.edx;

                //printf("fd: %u\nbuf: 0x%x\ncount: %u\n", fd, buf, count);

                if (fd == STDOUT_FILENO) {
                    if (count == 1 && buf < 0xFF) {
                        terminal_writechar((char) buf, true);
                    } else {
                        terminal_write((void*) buf, count);
                    }
                } else {
                    printf("Invalid fd '%d'\n", fd);
                }
            }

            break;

        case (SYS_yield):
            {
                //printf("\nYield!\n");

                // Don't know why kernel would call yield but just in case
                //if (ss & 0x3) {
                if (!(frame.cs & 0x3)) {
                    printf("[INFO] Kernel called yield syscall?\n");
                    for (;;) {}
                }

                // Save registers
                process_t* process = getCurrentProcess();
                saveRegisters(process, &stack_state, &frame, esp);

                // Switch to next process
                switchProcess();
            }

            break;

        case (SYS_read):
            {
                //if (!(ss & 0x3)) {
                if (!(frame.cs & 0x3)) {
                    printf("[INFO] Kernel called read syscall syscall?\n");
                    for (;;) {}
                }

                unsigned int fd = stack_state.ebx;

                if (fd == STDIN_FILENO) {
                    process_t* process = getCurrentProcess();
                    process->state = BLOCKED_KEYBOARD;

                    process->blocked_regs.eax = stack_state.eax;
                    process->blocked_regs.ebx = stack_state.ebx;
                    process->blocked_regs.ecx = stack_state.ecx;
                    process->blocked_regs.edx = stack_state.edx;
                    process->blocked_regs.ebp = stack_state.ebp;
                    process->blocked_regs.edi = stack_state.edi;
                    process->blocked_regs.esi = stack_state.esi;

                    // Save registers
                    saveRegisters(process, &stack_state, &frame, esp);

                    // Number of bytes read
                    process->regs.eax = 0;

                    // Switch to next process
                    switchProcess();
                } else {
                    printf("Invalid fd '%d'\n", fd);
                }
            }

            break;

        case (SYS_fork):
            {
                if (!(frame.cs & 0x3)) {
                    printf("[ERROR] Kernel called fork syscall?\n");
                    for (;;) {}
                }

                process_t* process = getCurrentProcess();

                // Save registers
                saveRegisters(process, &stack_state, &frame, esp);

                forkProcess(process);

                // Since we have changed registers in current process we
                //  can't simply iret, but also load registers
                runCurrentProcess();
            }
            break;

        case (SYS_waitpid):
            {
                //if (!(ss & 0x3)) {
                if (!(frame.cs & 0x3)) {
                    printf("[ERROR] Kernel called waitpid?\n");
                    for (;;) {}
                }

                process_t* process = getCurrentProcess();
                process->state = BLOCKED_WAITPID;

                process->blocked_regs.eax = stack_state.eax;
                process->blocked_regs.ebx = stack_state.ebx;
                process->blocked_regs.ecx = stack_state.ecx;
                process->blocked_regs.edx = stack_state.edx;
                process->blocked_regs.ebp = stack_state.ebp;
                process->blocked_regs.edi = stack_state.edi;
                process->blocked_regs.esi = stack_state.esi;

                // Save registers
                saveRegisters(process, &stack_state, &frame, esp);

                // Switch to next process
                switchProcess();
            }

            break;

        case (SYS_execvp):
            {
                //if (!(ss & 0x3)) {
                if (!(frame.cs & 0x3)) {
                    printf("[ERROR] Kernel called execvp?\n");
                    for (;;) {}
                }

                char* file = (char*) stack_state.ebx;
                const char** argv = (const char**) stack_state.ecx;

                process_t* process = getCurrentProcess();

                // Save registers incase overwriteArgs fails
                saveRegisters(process, &stack_state, &frame, esp);

                int result = overwriteArgs(process, file, argv);

                if (result == -1) {

                    // Indicate error
                    process->regs.eax = -1;

                }

                // Since we have changed entire process then we need
                //  to reload those changes
                runCurrentProcess();
            }

            break;


        default:
            printf("Invalid syscall id '%d'\n", stack_state.eax);
    }
}

//void interrupt_handler(stack_state_t stack_state, test_struct_t test_struct, unsigned int interrupt_id, interrupt_frame_t frame) {
void interrupt_handler(test_struct_t test_struct, unsigned int interrupt_id, stack_state_t stack_state, interrupt_frame_t frame, unsigned int esp, unsigned int ss) {
    //printf("\nInterrupt handler:\n");

    //const char* formatted = format_interrupt(interrupt_id);

    (void)stack_state;
    (void)test_struct;
    (void)frame;
    (void)ss;
    (void)esp;

    unsigned int irq = interrupt_id - IDT_IRQ_OFFSET;

    /*
    printf(" - Interrupt: %s\n", formatted);
    printf(" - Interrupt id: '%d'\n", interrupt_id);
    printf(" - IRQ: '%d'\n", irq);
    */

    /*
    printf(" - edi: '%d'\n", stack_state.edi);
    printf(" - eax: '%d'\n", stack_state.eax);
    printf(" - esp: '%d'\n", stack_state.esp);
    printf(" - Test1: '%d'\n", test_struct.num1);
    printf(" - Test2: '%d'\n", test_struct.num2);
    printf(" - eflags: '%d'\n", frame.eflags);
    printf(" - cs: '%d'\n", frame.cs);
    printf(" - eip: '%d'\n", frame.eip);
    */

#define KEY_LEFT_SHIFT  1
#define KEY_RIGHT_SHIFT 2
#define KEY_LEFT_CTRL   3
#define KEY_ALT         4
#define KEY_F1          5
#define KEY_F2          6
#define KEY_F3          7
#define KEY_F4          8
#define KEY_F5          9
#define KEY_F6          10
#define KEY_F7          11
#define KEY_F8          12
#define KEY_F9          13
#define KEY_F10         14
#define KEY_F11         15
#define KEY_F12         16
#define KEY_CAPS_LOCK   17
#define KEY_LEFT_ARROW  18
#define KEY_RIGHT_ARROW 19
#define KEY_UP_ARROW    20
#define KEY_DOWN_ARROW  21


    char keyboard_special_US[128] =
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        KEY_LEFT_CTRL, /* <-- control key */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KEY_LEFT_SHIFT, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KEY_RIGHT_SHIFT,
        0,
        KEY_ALT,  /* Alt */
        0,  /* Space bar */
        KEY_CAPS_LOCK,  /* Caps lock */
        KEY_F1,  /* 59 - F1 key ... > */
        KEY_F2,   KEY_F3,   KEY_F4,   KEY_F5,   KEY_F6,   KEY_F7,   KEY_F8,   KEY_F9,
        KEY_F10,  /* < ... F10 */
        0,  /* 69 - Num lock*/
        0,  /* Scroll Lock */
        0,  /* Home key */
        KEY_UP_ARROW,  /* Up Arrow */
        0,  /* Page Up */
        0,
        KEY_LEFT_ARROW,  /* Left Arrow */
        0,
        KEY_RIGHT_ARROW,  /* Right Arrow */
        0,
        0,  /* 79 - End key*/
        KEY_DOWN_ARROW,  /* Down Arrow */
        0,  /* Page Down */
        0,  /* Insert Key */
        0,  /* Delete Key */
        0,   0,   0,
        0,  /* F11 Key */
        0,  /* F12 Key */
        0,  /* All other keys are undefined */
    };

    //io_outb(PIC1, PIC_EOI);
    //io_outb(PIC2, PIC_EOI);
    char keyboard_US[128] =
    {
        0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
      '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, /* <-- control key */
      'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',  0, '\\',
      'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',  0,
      '*',
        0,  /* Alt */
      ' ',  /* Space bar */
        0,  /* Caps lock */
        0,  /* 59 - F1 key ... > */
        0,   0,   0,   0,   0,   0,   0,   0,
        0,  /* < ... F10 */
        0,  /* 69 - Num lock*/
        0,  /* Scroll Lock */
        0,  /* Home key */
        24,  /* Up Arrow */
        0,  /* Page Up */
      '-',
        27,  /* Left Arrow */
        0,
        26,  /* Right Arrow */
      '+',
        0,  /* 79 - End key*/
        25,  /* Down Arrow */
        0,  /* Page Down */
        0,  /* Insert Key */
        0,  /* Delete Key */
        0,   0,   0,
        0,  /* F11 Key */
        0,  /* F12 Key */
        0,  /* All other keys are undefined */
    };

    char keyboard_shift_US[128] =
    {
        0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
      '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
        0, /* <-- control key */
       'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',  0, '|',
       'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',  0,
       '*',
        0,  /* Alt */
       ' ',  /* Space bar */
        0,  /* Caps lock */
        0,  /* 59 - F1 key ... > */
        0,   0,   0,   0,   0,   0,   0,   0,
        0,  /* < ... F10 */
        0,  /* 69 - Num lock*/
        0,  /* Scroll Lock */
        0,  /* Home key */
        24,  /* Up Arrow */
        0,  /* Page Up */
       '-',
        27,  /* Left Arrow */
        0,
        26,  /* Right Arrow */
       '+',
        0,  /* 79 - End key*/
        25,  /* Down Arrow */
        0,  /* Page Down */
        0,  /* Insert Key */
        0,  /* Delete Key */
        0,   0,   0,
        0,  /* F11 Key */
        0,  /* F12 Key */
        0,  /* All other keys are undefined */
    };

    if (interrupt_id == INT_KEYBOARD) {
        int scancode = io_inb(0x60);
        //int type = io_inb(0x61);
        //io_outb(0x61, i|0x80);
        //io_outb(0x61, i);
        //io_outb(PIC1, 0x20);

        //if (type == KEY_PRESSED) {
        //}

        key_state_t state;

        if (scancode < 128)
            state = PRESSED;
        else {
            state = RELEASED;
            scancode -= 128;
        }


        char key = keyboard_US[scancode];

        // Check for special key
        if (key == 0) {

            key = keyboard_special_US[scancode];

            if (key == KEY_LEFT_SHIFT || key == KEY_RIGHT_SHIFT) {

                if (state == PRESSED)
                    shiftDown = true;
                else
                    shiftDown = false;

            } else if (key == KEY_CAPS_LOCK) {

                if (state == PRESSED)
                    capslock = !capslock;

            } else {
                // Not-implemented yet
            }

        } else if (state == PRESSED) {

            if (shiftDown || capslock)
                key = keyboard_shift_US[scancode];

            handleKeyboardBlock(key);
        }

        PIC_sendEOI(irq);

    } else if (interrupt_id == INT_TIMER) {
        tickCounter += 1;

        PIC_sendEOI(irq);

        if (tickCounter == 3) {
        //if (tickCounter == 50) {
            tickCounter = 0;

            VERBOSE("INT_TIMER: Task switch!\n");

            /*
            printf(" - cs: 0x%x\n", frame.cs);

            uint32_t cs;
            asm volatile("mov %%cs, %0" : "=r"(cs));
            printf(" - current cs: 0x%x\n", cs);
            */

            // Only save registers if we came from userspace
            if (frame.cs & 0x3) {

                VERBOSE("INT_TIMER: Came from userspace\n");

                // Save registers
                process_t* process = getCurrentProcess();
                if (process->initialized) {
                    VERBOSE("INT_TIMER: Saving registers for %d:%s\n", process->id, process->name);

                    saveRegisters(process, &stack_state, &frame, esp);
                }
            }

            // Switch to next process
            switchProcess();
        }
    } else {
        VERBOSE("Got unknown IRQ %d\n", interrupt_id);
        printf("Unknown IRQ %d\n", interrupt_id);
        for (;;) {}
    }
}

void exception_handler(unsigned int cr2, test_struct_t test_struct, unsigned int interrupt_id, stack_state_t stack_state, bool has_error, unsigned int error_code, interrupt_frame_t frame, unsigned int esp, unsigned int ss) {

    printf("\nException handler:\n");

    //for (;;){}

    const char* formatted = format_interrupt(interrupt_id);

    (void)stack_state;
    (void)test_struct;
    (void)frame;
    (void)has_error;

    printf(" - Interrupt: %s\n", formatted);
    printf(" - Interrupt id: '%d'\n\n", interrupt_id);

    /*
    if (interrupt_id == 0x08) {
        return;
    }
    */

    printf(" - eax: '%d'\n", stack_state.eax);
    /*
    printf(" - ebx: '%d'\n", ebx);
    printf(" - ecx: '%d'\n", ecx);
    printf(" - edx: '%d'\n", edx);
    printf(" - ebp: '%d'\n", ebp);
    printf(" - edi: '%d'\n", edi);
    printf(" - esi: '%d'\n\n", esi);
    */
    printf(" - Test1: '%d'\n", test_struct.num1);
    printf(" - Test2: '%d'\n", test_struct.num2);
    printf(" - Error code: '%d'\n", error_code);
    printf(" - eflags: 0x%x\n", frame.eflags);
    printf(" - cs: 0x%x\n", frame.cs);
    printf(" - current esp: 0x%x\n", stack_state.esp);
    printf(" - faulted from ring %d\n", frame.cs & 0x3);

    printf(" - program eip: 0x%x\n", frame.eip);
    if (frame.cs & 0x3) {
        printf(" - program esp: 0x%x\n", esp);
        printf(" - program ss: 0x%x\n", ss);
    }

    uint32_t cs;
    asm volatile("mov %%cs, %0" : "=r"(cs));
    printf(" - current cs: 0x%x\n", cs);

    if (interrupt_id == INT_GENERAL_PROTECTION) {
        printf("\nError breakdown:\n");


        /* Volume 3 - Chapter 6.13 & 6.14 */

        /** EXT
         * External event (bit 0) — When set, indicates that the exception occurred during delivery of an
         * event external to the program, such as an interrupt or an earlier exception. 5 The bit is cleared if the
         * exception occurred during delivery of a software interrupt (INT n, INT3, or INTO).
        */

        printf(" - External event: '%d'\n", (error_code & 0x01) > 0);

        /** IDT
         * Descriptor location (bit 1) — When set, indicates that the index portion of the error code refers
         * to a gate descriptor in the IDT; when clear, indicates that the index refers to a descriptor in the GDT
         * or the current LDT.
         */

        printf(" - Descriptor location: '%d'\n", (error_code & 0x02) > 0);


        /** TI
         * GDT/LDT (bit 2) — Only used when the IDT flag is clear. When set, the TI flag indicates that the
         * index portion of the error code refers to a segment or gate descriptor in the LDT; when clear, it indi-
         * cates that the index refers to a descriptor in the current GDT.
         */

        if (!(error_code & 0x02)) {
            printf(" - GDT / LDT: '%d'\n", (error_code & 0x04) > 0);
        }

        /** Segment selector index
         * The segment selector index field provides an index into the IDT, GDT, or current LDT to the segment or gate
         * selector being referenced by the error code. In some cases the error code is null (all bits are clear except possibly
         * EXT). A null error code indicates that the error was not caused by a reference to a specific segment or that a null
         * segment selector was referenced in an operation.
         */

        printf(" - Segment Selector Index: '%d'\n", (error_code >> 3) & 0xFF);

        __asm__ volatile ("cli; hlt"); // Completely hangs the computer
    } else if (interrupt_id == INT_PAGE_FAULT) {
        printf("\nError breakdown:\n");

        printf(" - Fault happened because of operation at 0x%x\n", cr2);
        
        pagedirectory_t pd;
        if (error_code & 0x04) {
            pd = getCurrentProcess()->pd;
        } else {
            pd = page_directory;
        }

        uint32_t virtualPTI = cr2 / FRAME_4MB;
        uint32_t virtualPI = (cr2 / FRAME_4KB) & 0x03FF;

        printf(" - PTI: %d, PI: %d\n", virtualPTI, virtualPI);

        bool present = pd[virtualPTI] & 0x1;
        bool rw = (pd[virtualPTI] & 0x2) > 0;
        bool kernel = !(pd[virtualPTI] & 0x4);

        printf(" - Pagetable is 0x%x with flags p%d rw%d and k%d\n", pd[virtualPTI], present, rw, kernel);
        if (pd[virtualPTI] & 1) {
            pagetable_t pagetable = (pagetable_t) p_to_v((pd[virtualPTI] & 0xFFFFF000));
            uint32_t page = pagetable[virtualPI];

            present = page & 0x1;
            rw = (page & 0x2) > 0;
            kernel = !(page & 0x4);

            printf(" - Page is 0x%x with flags p%d rw%d and k%d\n", page, present, rw, kernel);
        } else {
            printf(" - Pagetable not present\n");
        }

        if (error_code & 0x01) {
            printf(" - Page was present\n");
        } else {
            printf(" - Page wasn't present\n");
        }

        if (error_code & 0x02) {
            printf(" - Operation that caused fault was a write-operation\n");
        } else {
            printf(" - Operation that caused fault was a read-operation\n");
        }

        if (error_code & 0x04) {
            printf(" - Fault happened in user-mode\n");

            process_t* process = getCurrentProcess();
            printf(" - Current process has id '%d' and name '%s'\n", process->id, process->name);
        } else {
            printf(" - Fault happened in kernel-mode\n");
        }

        if (error_code & 0x08) {
            printf(" - Fault caused by reserved bits being overwritten\n");
        }

        if (error_code & 0x10) {
            printf(" - Fault occured because of instruction fetch\n");
        }

        if (error_code & 0x20) {
            printf(" - Fault caused by protection-key violation\n");
        }

        if (error_code & 0x40) {
            printf(" - Fault caused by shadow-stack access\n");
        }

        if (error_code & 0x80) {
            printf(" - Fault occured during HLAT paging\n");
        }

        if (error_code & 0x8000) {
            printf(" - Fault resulted from violation of SGX-specific access-control requirements\n");
        }

        // Ensure that the error wasn't because of an instruction fetch
        if (!(error_code & 0x10)) {
            // If page is trying to be read by user
            if (error_code & 0x04) {

            } else {

            }
        }

        __asm__ volatile ("cli; hlt"); // Completely hangs the computer
    } else {
        __asm__ volatile ("cli; hlt"); // Completely hangs the computer
    }
}

void idt_initialize(void) {
    idtr.base = (uintptr_t) &idt[0];
    idtr.limit = (uint16_t) sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;
    
    for (size_t vector = 0; vector < IDT_PRIVILEGED_DESCRIPTORS; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        //vectors[vector] = true;
    }

    // Syscall
    //idt_set_descriptor(INT_SYSCALL, isr_stub_table[INT_SYSCALL], 0x8E);
    //idt_set_descriptor(INT_SYSCALL, isr_stub_table[INT_SYSCALL], 0xE5);
    idt_set_descriptor(INT_SYSCALL, isr_stub_table[INT_SYSCALL], 0xEE);

    PIC_remap(IDT_IRQ_OFFSET, IDT_IRQ_OFFSET + 0x08);

    //load_idt(idtr);
    __asm__ volatile ("lidt %0" : : "m"(idtr)); // load the new IDT
    //__asm__ volatile ("sti"); // set the interrupt flag

    io_outb(PIC1_DATA, 0xFD);
    io_outb(PIC2_DATA, 0xFF);
    io_enable();
}


void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    idt_entry_t* descriptor = &idt[vector];

    descriptor->isr_low = ((uint32_t) isr) & 0xFFFF;
    descriptor->kernel_cs = 0x08; // this value is whatever offset your kernel code selector is in your GDT
    descriptor->attributes = flags;
    descriptor->isr_high = ((uint32_t) isr) >> 16;
    descriptor->reserved = 0;
}

void PIC_sendEOI(unsigned char irq)
{
    if(irq >= 8) {
        io_outb(PIC2_COMMAND, PIC_EOI);
    }

    io_outb(PIC1_COMMAND, PIC_EOI);
}

/* Copied from https://wiki.osdev.org/PIC#Initialisation */
void PIC_remap(int offset1, int offset2) {
    unsigned char a1;
    unsigned char a2;

    a1 = io_inb(PIC1_DATA);
    a2 = io_inb(PIC2_DATA);

    io_outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    io_outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    io_outb(PIC1_DATA, offset1);
    io_wait();
    io_outb(PIC2_DATA, offset2);
    io_wait();

    io_outb(PIC1_DATA, 4);
    io_wait();
    io_outb(PIC2_DATA, 2);
    io_wait();

    io_outb(PIC1_DATA, ICW4_8086);
    io_wait();
    io_outb(PIC2_DATA, ICW4_8086);
    io_wait();

    io_outb(PIC1_DATA, a1);
    io_outb(PIC2_DATA, a2);
}


bool irq_read_mask(unsigned char line) {

    uint16_t port;

    if (line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        line -= 8;
    }

    return (io_inb(port) & (1 << line)) > 0;
}

void irq_write_mask(unsigned char line, bool set) {

    uint16_t port;
    uint8_t value;

    if (line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        line -= 8;
    }

    if (set) {
        value = io_inb(port) | (1 << line);
    } else {
        value = io_inb(port) & ~(1 << line);
    }

    io_outb(port, value);
}

void irq_set_mask(unsigned char line) {

    uint16_t port;
    uint8_t value;

    if (line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        line -= 8;
    }

    value = io_inb(port) | (1 << line);
    io_outb(port, value);
}

void irq_clear_mask(unsigned char line) {

    uint16_t port;
    uint8_t value;

    if (line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        line -= 8;
    }

    value = io_inb(port) & ~(1 << line);
    io_outb(port, value);
}

void irq_store_mask(unsigned char line) {

    maskBackup = irq_read_mask(line);
}

void irq_restore_mask(unsigned char line) {

    irq_write_mask(line, maskBackup);
}

void pit_set_count(unsigned count) {

    // Disable interrupts
    cli();

    io_outb(0x40, count & 0xFF);           // Low byte
    io_outb(0x40, (count & 0xFF00) >> 8);  // High byte
}

