
; Declare some constants for the multiboot header
MBALIGN  equ 1 << 0
MEMINFO  equ 1 << 1
MBFLAGS  equ MBALIGN | MEMINFO
MAGIC    equ 0x1BADB002
CHECKSUM equ -(MAGIC + MBFLAGS)


; Set the bytes
section .multiboot
align 4
    dd MAGIC
    dd MBFLAGS
    dd CHECKSUM



; Reserve 16kb for stack
section .bss
align 16
stack_bottom:
    resb 16384
stack_top:


; _start is the entry point to the kernel.
; Also say how big the function is.
section .text
global _start:function (_start.end - _start)
_start:

    ; Now we are in 32-bit real-mode.

    ; Set the esp register to the top of the stack as it grows downwards.
    mov esp, stack_top

    ; Initialize crucial stuff here.
    ; The GDT should be loaded here and paging should be enabled here.

    ; Enter the kernel!
    extern kernel_main
    call kernel_main

    ; Kernel shouldn't return, but if it does, then loop forever
    cli
.hang:
    hlt
    jmp .hang
.end:


