
#include <stdio.h>
#include <stdint.h>
#include <string.h>

struct GDT {
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
} __attribute__((packed));

int main(void) {
    struct GDT gdt;
    gdt.limit_low = 0xFFFF;
    gdt.base_low = 0;
    gdt.accessed = 0;
    gdt.read_write = 1; // since this is a code segment, specifies that the segment is readable
    gdt.conforming = 0; // does not matter for ring 3 as no lower privilege level exists
    gdt.code = 1;
    gdt.code_data_segment = 1;
    gdt.DPL = 3; // ring 3
    gdt.present = 1;
    gdt.limit_high = 0xF;
    gdt.available = 1;
    gdt.long_mode = 0;
    gdt.big = 1; // it's 32 bits
    gdt.gran = 1; // 4KB page addressing
    gdt.base_high = 0;
}

