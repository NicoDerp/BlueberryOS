
#include <kernel/gdt.h>
#include <kernel/usermode.h>
#include <kernel/errors.h>
#include <stdio.h>


//extern void load_gdt(uint16_t limit, uint32_t base);
extern void load_gdt(uint8_t limit, uint8_t* base);
//extern void load_gdt(uint8_t* entry);
extern void reload_segments();
void gdt_entry(uint8_t* target, struct GDT source);


uint8_t gdt[6][8]; // TODO 6 * 8 with TSS


void gdt_initialize() {

    struct GDT source;

    /* 0x00 */
    /* Null Descriptor */
    source.base = 0;
    source.limit = 0x00000;
    source.access_byte = 0x00;
    source.flags = 0x0;
    gdt_entry(gdt[0], source);

    /* 0x08 */
    /* Kernel Mode Code Segment */
    source.base = 0;
    source.limit = 0xFFFFF;
    source.access_byte = 0x9A;
    source.flags = 0xC;
    gdt_entry(gdt[1], source);

    /* 0x10 */
    /* Kernel Mode Data Segment */
    source.base = 0;
    source.limit = 0xFFFFF;
    source.access_byte = 0x92;
    source.flags = 0xC;
    gdt_entry(gdt[2], source);

    /* 0x18 */
    /* User Mode Code Segment */
    source.base = 0;
    source.limit = 0xFFFFF;
    source.access_byte = 0xFA;
    source.flags = 0xC;
    gdt_entry(gdt[3], source);

    /* 0x20 */
    /* User Mode Data Segment */
    source.base = 0;
    source.limit = 0xFFFFF;
    source.access_byte = 0xF2;
    source.flags = 0xC;
    gdt_entry(gdt[4], source);

    /* 0x28 */
    /* Task State Segment */
    //install_tss(&source);
    //gdt_entry(gdt[5], source);
    install_tss(gdt[5]);

    //printf("Running %d, %d\n", sizeof(gdt), &gdt[0]);

    load_gdt(sizeof(gdt)-1, &gdt[0][0]);
    reload_segments();

}

void gdt_entry(uint8_t* target, struct GDT source) {
    // Check the limit to make sure that it can be encoded
    if (source.limit > 0xFFFFF) {
        kerror("GDT cannot encode limits larger than 0xFFFFF");
    }

    // Encode the limit
    target[0] = source.limit & 0xFF;
    target[1] = (source.limit >> 8) & 0xFF;
    target[6] = (source.limit >> 16) & 0x0F;

    // Encode the base
    target[2] = source.base & 0xFF;
    target[3] = (source.base >> 8) & 0xFF;
    target[4] = (source.base >> 16) & 0xFF;
    target[7] = (source.base >> 24) & 0xFF;

    // Encode the access byte
    target[5] = source.access_byte;

    // Encode the flags
    target[6] |= (source.flags << 4);
}

