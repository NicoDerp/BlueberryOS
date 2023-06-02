
/* The bootloader will look at this image and start execution at the symbol
 * designated as the entry point */

ENTRY(_start)



/* Tell where the various sections of the object files will be put in
 * the final kernel image. */

SECTIONS {

    /* Instead of placing the kernel at 1mb, we now place it at 3gb + 1mb!
     * This means a higher-half kernel. */
    . = 0xC0100000;


    /* First put the multiboot header, as it is required to be put very early
     * in the image, or the bootloader won't recognize the file format */
    .text BLOCK(4K) : ALIGN(4K) : AT(ADDR(.text)-0xC0100000) {
        *(.multiboot)
        *(.text)
    }


    /* Read-only data */
    .rodata BLOCK(4K) : ALIGN(4K) : AT(ADDR(.text)-0xC0100000) {
        *(.rodata)
    }


    /* Read-write data (initialized) */
    .data BLOCK(4K) : ALIGN(4K) : AT(ADDR(.text)-0xC0100000) {
        *(.data)
    }


    /* Read-write data (uninitialized) and stack */
    .bss BLOCK(4K) : ALIGN(4K) : AT(ADDR(.text)-0xC0100000) {
        *(COMMON)
        *(.bss)
    }


    /* The compiler may produce other sections, by default it will put them
     * in a segment with the same name. Simply add stuff here as needed. */

}

