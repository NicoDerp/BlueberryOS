
#ifndef KERNEL_IDT_H
#define KERNEL_IDT_H

#include <stdint.h>

#define IDT_IRQ_OFFSET 0x20

#define IDT_DESCRIPTORS 32 + 16         /* First 32 are CPU. Next 16 are PIC. */
#define IDT_MAX_DESCRIPTORS 256

#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)

#define PIC_EOI		0x20		/* End-of-interrupt command code */

#define ICW1_ICW4	0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */
 
#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */


#define INT_INVALID_OPCODE      6
#define INT_DOUBLE_FAULT        8
#define INT_GENERAL_PROTECTION  13
#define INT_PAGE_FAULT          14

#define INT_TIMER               IDT_IRQ_OFFSET + 0
#define INT_KEYBOARD            IDT_IRQ_OFFSET + 1
#define INT_MOUSE               IDT_IRQ_OFFSET + 12

#define INT_SYSCALL


#define KEY_PRESSED     0x30
#define KEY_RELEASED    0x20



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
    unsigned int edi;
    unsigned int esi;
    unsigned int ebp;
    unsigned int esp;
    unsigned int ebx;
    unsigned int edx;
    unsigned int ecx;
    unsigned int eax;
} __attribute__((packed)) stack_state_t;

typedef struct {
    unsigned int eflags;
    unsigned int eip;
    unsigned int cs;
} __attribute__((packed)) interrupt_frame_t;

#endif /* KERNEL_IDT_H */

