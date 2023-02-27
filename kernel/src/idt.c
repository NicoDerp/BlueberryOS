
#include <kernel/idt.h>
#include <kernel/io.h>

#include <stdbool.h>
#include <stdio.h>

// Function prototypes
void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags);
//__attribute__((noreturn)) void exception_handler(void);
void interrupt_handler(uint8_t, uint8_t);
extern void load_idt(idtr_t);


// Create an array of IDT entries; aligned for performance
__attribute__((aligned(0x10))) static idt_entry_t idt[IDT_DESCRIPTORS];

// Create an IDTR
static idtr_t idtr;

// Table of all exceptions. Defined in idt.asm
extern void* isr_stub_table[];

//static bool vectors[32];


const char* format_interrupt(uint8_t id) {
    if (id == INT_DOUBLE_FAULT) {
        return "DOUBLE_FAULT";
    } else {
        return "NOT IMPLEMENTED";
    }
}


void interrupt_handler(uint8_t interrupt_id, uint8_t is_err) {
    printf("\nInterrupt handler:\n");

    const char* formatted = format_interrupt(interrupt_id);

    printf(" - Interrupt: %s\n", formatted);
    printf(" - Interrupt id: '%d'\n", interrupt_id);
    printf(" - Is error: '%d'\n", is_err);
    printf("");

    if (is_err) {
        printf(" - Error!\n");
        __asm__ volatile ("cli; hlt"); // Completely hangs the computer
    }

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

    descriptor->isr_low        = ((uint32_t) isr) & 0xFFFF;
    descriptor->kernel_cs      = 0x08; // this value is whatever offset your kernel code selector is in your GDT
    descriptor->attributes     = flags;
    descriptor->isr_high       = ((uint32_t) isr) >> 16;
    descriptor->reserved       = 0;
}

