
#include <kernel/idt.h>
#include <kernel/io.h>

#include <stdbool.h>
#include <stdio.h>

// Function prototypes
void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags);
void PIC_remap(int offset1, int offset2);
void PIC_sendEOI(unsigned char irq);
extern void load_idt(idtr_t);


// Create an array of IDT entries; aligned for performance
__attribute__((aligned(0x10))) static idt_entry_t idt[IDT_DESCRIPTORS];

// Create an IDTR
static idtr_t idtr;

// Table of all exceptions. Defined in idt.asm
extern void* isr_stub_table[];

//static bool vectors[32];

const char* format_interrupt(uint8_t id) {
    if      (id == INT_INVALID_OPCODE) { return "INVALID_OPCODE"; }
    else if (id == INT_DOUBLE_FAULT) { return "DOUBLE_FAULT"; }
    else if (id == INT_GENERAL_PROTECTION) { return "GENERAL_PROTECTION"; }
    else if (id == INT_PAGE_FAULT) { return "PAGE_FAULT"; }
    else if (id == INT_TIMER) { return "TIMER"; }
    else if (id == INT_KEYBOARD) { return "KEYBOARD"; }
    else if (id == INT_MOUSE) { return "MOUSE"; }
    else { return "NOT IMPLEMENTED"; }
}

typedef struct {
    unsigned int num1; // arg1
    unsigned int num2; // arg2
} __attribute__((packed)) test_struct_t;

void interrupt_handler(stack_state_t stack_state, test_struct_t test_struct, unsigned int interrupt_id, interrupt_frame_t frame) {
    //printf("\nInterrupt handler:\n");

    //const char* formatted = format_interrupt(interrupt_id);

    (void)stack_state;
    (void)test_struct;
    (void)frame;

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

    //io_outb(PIC1, PIC_EOI);
    //io_outb(PIC2, PIC_EOI);
    char keyboard_US [128] =
    {
        0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
      '\t', /* <-- Tab */
      'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',     
        0, /* <-- control key */
      'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',  0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
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
        0,  /* Up Arrow */
        0,  /* Page Up */
      '-',
        0,  /* Left Arrow */
        0,
        0,  /* Right Arrow */
      '+',
        0,  /* 79 - End key*/
        0,  /* Down Arrow */
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

        if (scancode < 128) {
            //printf("type: %d, scan: %d\n", type, scancode);
            char key = keyboard_US[scancode];
            printf("%c", key);
        }

        PIC_sendEOI(irq);
    }
}

void exception_handler(stack_state_t stack_state, test_struct_t test_struct, unsigned int interrupt_id, interrupt_frame_t frame, unsigned int error_code, bool has_error) {
    printf("\nException handler:\n");


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
    printf(" - esp: '%d'\n", esp);
    printf(" - ebp: '%d'\n", ebp);
    printf(" - edi: '%d'\n", edi);
    printf(" - esi: '%d'\n\n", esi);
    */
    printf(" - Test1: '%d'\n", test_struct.num1);
    printf(" - Test2: '%d'\n", test_struct.num2);
    printf(" - Error code: '%d'\n", error_code);
    printf(" - eflags: '%d'\n", frame.eflags);
    printf(" - cs: '%d'\n", frame.cs);
    printf(" - eip: '%d'\n", frame.eip);

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
    } else if (interrupt_id == INT_PAGE_FAULT) {
        printf("\nError breakdown:\n");
        
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
        } else {
            printf(" - Fault happened in kernel-mode\n");
        }

        if (error_code & 0x08) {
            printf(" - Fault caused by reserved bits being overwritten\n");
        }

        if (error_code & 0x10) {
            printf(" - Fault occured during instruction fetch\n");
        }
    }

    __asm__ volatile ("cli; hlt"); // Completely hangs the computer
}

void common_handler(stack_state_t stack_state, test_struct_t test_struct, unsigned int interrupt_id, bool has_error, interrupt_frame_t frame, unsigned int error_code) {

    if (interrupt_id < 32) {
        exception_handler(stack_state, test_struct, interrupt_id, frame, error_code, has_error);
    } else {
        interrupt_handler(stack_state, test_struct, interrupt_id, frame);
    }
}

void idt_initialize(void) {
    idtr.base = (uintptr_t) &idt[0];
    idtr.limit = (uint16_t) sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;
    
    for (size_t vector = 0; vector < IDT_DESCRIPTORS; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        //vectors[vector] = true;
    }

    PIC_remap(IDT_IRQ_OFFSET, IDT_IRQ_OFFSET + 0x08);

    //load_idt(idtr);
    __asm__ volatile ("lidt %0" : : "m"(idtr)); // load the new IDT
    __asm__ volatile ("sti"); // set the interrupt flag

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


