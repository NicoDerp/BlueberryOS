
#ifndef KERNEL_GDT_H
#define KERNEL_GDT_H

#include <stdint.h>


struct GDT {
    uint32_t base;
    uint32_t limit;
    uint8_t access_byte;
    uint8_t flags;
};

void gdt_initialize(void);
void gdt_entry(uint8_t* target, struct GDT source);


#endif /* KERNEL_GDT_H */

