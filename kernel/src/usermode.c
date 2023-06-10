
#include <kernel/usermode.h>
#include <kernel/gdt.h>
#include <string.h>
#include <stdio.h>


tss_t sys_tss;

typedef struct {
	unsigned int limit_low              : 16;
	unsigned int base_low               : 24;
	unsigned int accessed               :  1;
	unsigned int read_write             :  1; // readable for code, writable for data
	unsigned int conforming_expand_down :  1; // conforming for code, expand down for data
	unsigned int code                   :  1; // 1 for code, 0 for data
	unsigned int code_data_segment      :  1; // should be 1 for everything but TSS and LDT
	unsigned int DPL                    :  2; // privilege level
	unsigned int present                :  1;
	unsigned int limit_high             :  4;
	unsigned int available              :  1; // only used in software; has no effect on hardware
	unsigned int long_mode              :  1;
	unsigned int big                    :  1; // 32-bit opcodes for code, uint32_t stack for data
	unsigned int gran                   :  1; // 1 to use 4k page addressing, 0 for byte addressing
	unsigned int base_high              :  8;
} __attribute__((packed)) gdt_entry_t;

void tss_initialize(void) {

    // Ensure tss is zeroed
    memset(&sys_tss, 0, sizeof(tss_t));

    sys_tss.ss0 = 0x10;  /* Kernel data segment */
    sys_tss.esp0 = 0x00; /* Kernel stack pointer offset idk what to put here */

    sys_tss.cs = 0x08;   /* Kernel code segment */

    //sys_tss.iomap = (unsigned short) sizeof(tss_t);
}

/*
void install_tss(uint8_t* entryBytes) {

    gdt_entry_t* entry = (gdt_entry_t*) entryBytes;

    uint32_t base = (uint32_t) &sys_tss;
    uint32_t limit = sizeof(tss_t);

    entry->limit_low = limit;
    entry->base_low = base;
    entry->accessed = 1; // With a system entry (`code_data_segment` = 0), 1 indicates TSS and 0 indicates LDT
    entry->read_write = 0; // For a TSS, indicates busy (1) or not busy (0).
    entry->conforming_expand_down = 0; // always 0 for TSS
    entry->code = 1; // For a TSS, 1 indicates 32-bit (1) or 16-bit (0).
    entry->code_data_segment=0; // indicates TSS/LDT (see also `accessed`)
    entry->DPL = 0; // ring 0, see the comments below
    entry->present = 1;
    entry->limit_high = (limit & (0xf << 16)) >> 16; // isolate top nibble
    entry->available = 0; // 0 for a TSS
    entry->long_mode = 0;
    entry->big = 0; // should leave zero according to manuals.
    entry->gran = 0; // limit is in bytes, not pages
    entry->base_high = (base & (0xff << 24)) >> 24; //isolate top byte

}
*/

/*
void install_tss(struct GDT* source) {

    source->access_byte = 0x89;
    source->flags = 0x0;
    source->base = (uint32_t) &sys_tss;
    source->limit = sizeof(tss_t);
}
*/

void install_tss(uint8_t* gdt) {

    struct GDT source;
    source.access_byte = 0x89;
    source.flags = 0x0;
    source.base = (uint32_t) &sys_tss;
    source.limit = sizeof(tss_t);

    gdt_entry(gdt, source);
}

void set_kernel_stack(uint32_t esp) {
    // Setting ss0 just in case
    sys_tss.ss0 = 0x10;  /* Kernel data segment */
    sys_tss.esp0 = esp;
}


