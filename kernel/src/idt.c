
#include <kernel/idt.h>
#include <kernel/io.h>

#include <stdbool.h>
#include <stdio.h>

// Function prototypes
void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags);
//__attribute__((noreturn)) void exception_handler(void);
//void interrupt_handler(uint8_t, uint8_t, uint8_t);
extern void load_idt(idtr_t);


// Create an array of IDT entries; aligned for performance
__attribute__((aligned(0x10))) static idt_entry_t idt[IDT_DESCRIPTORS];

// Create an IDTR
static idtr_t idtr;

// Table of all exceptions. Defined in idt.asm
extern void* isr_stub_table[];

//static bool vectors[32];

const char* format_interrupt(uint8_t id) {
    if (id == INT_DOUBLE_FAULT) { return "DOUBLE_FAULT"; }
    else if (id == INT_GENERAL_PROTECTION) { return "GENERAL_PROTECTION"; }
    else { return "NOT IMPLEMENTED"; }
}

typedef struct {
    unsigned int num1; // arg1
    unsigned int num2; // arg2
} __attribute__((packed)) test_struct_t;

void interrupt_handler(stack_state_t stack_state, unsigned int test, unsigned int interrupt_id, interrupt_frame_t frame) {
//void interrupt_handler(interrupt_frame_t frame, stack_state_t stack_state, unsigned int interrupt_id) {
    printf("\nInterrupt handler:\n");

    const char* formatted = format_interrupt(interrupt_id);

    printf(" - Interrupt: %s\n", formatted);
    printf(" - Interrupt id: '%d'\n", interrupt_id);
    printf(" - edi: '%d'\n", stack_state.edi);
    printf(" - eax: '%d'\n", stack_state.eax);
    printf(" - esp: '%d'\n", stack_state.esp);
    printf(" - Test: '%d'\n", test);
    printf(" - eflags: '%d'\n", frame.eflags);
    printf(" - cs: '%d'\n", frame.cs);
    printf(" - eip: '%d'\n", frame.eip);

    __asm__ volatile ("cli; hlt"); // Completely hangs the computer
}

void exception_handler(stack_state_t stack_state, test_struct_t test_struct, unsigned int interrupt_id, unsigned int eflags, unsigned int cs, unsigned int eip, unsigned int error_code) {
//void exception_handler(interrupt_frame_t frame, stack_state_t stack_state, unsigned int error_code, unsigned int interrupt_id) {
    printf("\nException handler:\n");


    const char* formatted = format_interrupt(interrupt_id);

    printf(" - Interrupt: %s\n", formatted);
    printf(" - Interrupt id: '%d'\n\n", interrupt_id);

    /*
    if (interrupt_id == 0x08) {
        return;
    }
    */

    /*
    printf(" - eax: '%d'\n", eax);
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
    printf(" - eflags: '%d'\n", eflags);
    printf(" - cs: '%d'\n", cs);
    printf(" - eip: '%d'\n", eip);

    if (interrupt_id != 0x08) {
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
    }

    //__asm__ volatile ("cli; hlt"); // Completely hangs the computer
}

void idt_initialize(void) {
    idtr.base = (uintptr_t) &idt[0];
    idtr.limit = (uint16_t) sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;
    
    for (size_t vector = 0; vector < IDT_DESCRIPTORS; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        //vectors[vector] = true;
    }

    //load_idt(idtr);
    __asm__ volatile ("lidt %0" : : "m"(idtr)); // load the new IDT
    __asm__ volatile ("sti"); // set the interrupt flag

    //io_outb(0x21, 0xfd);
    //io_outb(0xa1, 0xff);
    //io_enable();
}


void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    idt_entry_t* descriptor = &idt[vector];

    descriptor->isr_low = ((uint32_t) isr) & 0xFFFF;
    descriptor->kernel_cs = 0x08; // this value is whatever offset your kernel code selector is in your GDT
    descriptor->attributes = flags;
    descriptor->isr_high = ((uint32_t) isr) >> 16;
    descriptor->reserved = 0;
}

