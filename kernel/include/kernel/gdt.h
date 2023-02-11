
#include <stdint.h>


struct GDT {
    uint32_t base;
    uint32_t limit;
    uint8_t access;
    uint8_t flags;
};

void initialize_gdt();


