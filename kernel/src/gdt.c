
#include <kernel/gdt.h>
#include <kernel/errors.h>


//extern void load_gdt(uint16_t limit, uint32_t base);
//extern void load_gdt(uint32_t limit, uint32_t base);
extern void load_gdt(uint8_t* entry);
void gdt_entry(uint8_t* target, struct GDT source);


void gdt_intialize() {

    struct GDT gdt;
    uint8_t entry[8];

    /* Null Descriptor */
    gdt.base = 0;
    gdt.limit = 0x00000000;
    gdt.access_byte = 0x00;
    gdt.flags = 0x0;
    gdt_entry((uint8_t*) entry, gdt);
    load_gdt(entry);

    /* Kernel Mode Code Segment */
    gdt.base = 0;
    gdt.limit = 0xFFFFF;
    gdt.access_byte = 0x9A;
    gdt.flags = 0xC;
    gdt_entry((uint8_t*) &entry, gdt);
    load_gdt(entry);

    /* Kernel Mode Data Segment */
    gdt.base = 0;
    gdt.limit = 0xFFFFF;
    gdt.access_byte = 0x92;
    gdt.flags = 0xC;
    gdt_entry((uint8_t*) &entry, gdt);
    load_gdt(entry);

    /* User Mode Code Segment */
    gdt.base = 0;
    gdt.limit = 0xFFFFF;
    gdt.access_byte = 0xFA;
    gdt.flags = 0xC;
    gdt_entry((uint8_t*) &entry, gdt);
    load_gdt(entry);

    /* User Mode Data Segment */
    gdt.base = 0;
    gdt.limit = 0xFFFFF;
    gdt.access_byte = 0xF2;
    gdt.flags = 0xC;
    gdt_entry((uint8_t*) &entry, gdt);
    load_gdt(entry);

    /*
    * Task State Segment *
    gdt.base = &tss;
    gdt.limit = sizeof(struct TSS);
    gdt.access_byte = 0x89;
    gdt.flags = 0x0;
    gdt_entry(&entry, gdt);
    load_gdt(entry);
    */

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

