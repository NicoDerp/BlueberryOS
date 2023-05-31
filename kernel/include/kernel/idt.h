
#ifndef KERNEL_IDT_H
#define KERNEL_IDT_H

#include <stdint.h>

#define IDT_DESCRIPTORS 32
#define IDT_MAX_DESCRIPTORS 256

#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)

#define PIC_EOI		0x20		/* End-of-interrupt command code */


#define INT_DOUBLE_FAULT        8
#define INT_GENERAL_PROTECTION  13


typedef struct {
    uint16_t isr_low; // Lower 16 bits of ISR's address
    uint16_t kernel_cs; // The GDT segment selector that the CPU will load into CS before called the ISR
    uint8_t reserved; // Reserved, set to zero
    uint8_t attributes; // Type and attributes
    uint16_t isr_high; // The higher 16 bits of ISR's address
} __attribute__((packed)) idt_entry_t;


typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idtr_t;


void idt_initialize(void);

typedef struct {
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;
    unsigned int esi;
    unsigned int edi;
    unsigned int ebp;
    unsigned int esp;
} __attribute__((packed)) stack_state_t;

typedef struct {
    unsigned int eip;
    unsigned int cs;
    unsigned int eflags;
} __attribute__((packed)) interrupt_frame_t;

#endif /* KERNEL_IDT_H */

