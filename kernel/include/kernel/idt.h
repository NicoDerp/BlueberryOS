
#ifndef KERNEL_IDT_H
#define KERNEL_IDT_H

#include <stdint.h>
#include <stdbool.h>

#define IDT_IRQ_OFFSET 0x20

#define IDT_USED_DESCRIPTORS 32 + 16 + 1     /* First 32 are CPU. Next 16 are PIC. Next is Syscall */
#define IDT_PRIVILEGED_DESCRIPTORS 32 + 16     /* First 32 are CPU. Next 16 are PIC. */
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

#define IRQ_TIMER       0
#define IRQ_KEYBOARD    1
#define IRQ_CASCADE     2
#define IRQ_COM2        3
#define IRQ_COM1        4
#define IRQ_LPT2        5
#define IRQ_FLOPPY      6
#define IRQ_LPT1        7
#define IRQ_CMOS        8
#define IRQ_MOUSE       12
#define IRQ_FPU         13
#define IRQ_PRIMARY_DISK 14
#define IRQ_SECONDARY_DISK 15

#define INT_INVALID_OPCODE      0x06  /* 6 */
#define INT_DOUBLE_FAULT        0x08  /* 8 */
#define INT_GENERAL_PROTECTION  0x0D  /* 13 */
#define INT_PAGE_FAULT          0x0E  /* 14 */

#define INT_TIMER               IDT_IRQ_OFFSET + IRQ_TIMER     /* 0 */
#define INT_KEYBOARD            IDT_IRQ_OFFSET + IRQ_KEYBOARD  /* 1 */
#define INT_MOUSE               IDT_IRQ_OFFSET + IRQ_MOUSE     /* 12 */

#define INT_SYSCALL             0x30  /* 48 */


typedef enum {
    PRESSED,
    RELEASED,
    HOLD,
} key_state_t;

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
void irq_set_mask(unsigned char line);
void irq_clear_mask(unsigned char line);
void pit_set_count(unsigned count);

bool irq_read_mask(unsigned char line);
void irq_write_mask(unsigned char line, bool value);

void irq_store_mask(unsigned char line);
void irq_restore_mask(unsigned char line);

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
    unsigned int eip;
    unsigned int cs;
    unsigned int eflags;
} __attribute__((packed)) interrupt_frame_t;

#endif /* KERNEL_IDT_H */

