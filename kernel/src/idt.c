
#include <kernel/idt.h>

#include <stdbool.h>
#include <stdio.h>


// Function prototypes
void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags);
//__attribute__((noreturn)) void exception_handler(void);
void exception_handler(void);
extern void load_idt(idtr_t);


// Create an array of IDT entries; aligned for performance
__attribute__((aligned(0x10))) static idt_entry_t idt[256];

// Create an IDTR
static idtr_t idtr;

// Table of all exceptions. Defined in idt.asm
extern void* isr_stub_table[];

static bool vectors[32];

// Default exception handler
void exception_handler() {
    printf("\nShiiish\n");
    //__asm__ volatile ("cli; hlt"); // Completely hangs the computer
}


void idt_initialize() {
    idtr.base = (uintptr_t) &idt[0];
    idtr.limit = (uint16_t) sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;
    
    for (uint8_t vector = 0; vector < 32; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = true;
    }

    load_idt(idtr);
}


void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    idt_entry_t* descriptor = &idt[vector];

    descriptor->isr_low        = (uint32_t)isr & 0xFFFF;
    descriptor->kernel_cs      = 0x08; // this value can be whatever offset your kernel code selector is in your GDT
    descriptor->attributes     = flags;
    descriptor->isr_high       = (uint32_t)isr >> 16;
    descriptor->reserved       = 0;
}




