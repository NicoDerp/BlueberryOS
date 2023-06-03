
; Declare some constants for the multiboot header
MBALIGN       equ 1 << 0
MEMINFO       equ 1 << 1
MBFLAGS       equ MBALIGN | MEMINFO
MAGIC         equ 0xE85250D6
ARCHITECTURE  equ 0  ; 32-bit protected
HEADER_LENGTH equ multiboot_header.end - multiboot_header
CHECKSUM      equ -(MAGIC + ARCHITECTURE + HEADER_LENGTH)


; Set the bytes
section .multiboot
align 4
multiboot_header:
    dd MAGIC
    dd ARCHITECTURE
    dd HEADER_LENGTH
    dd CHECKSUM

.frame_buffer:
    dw 5    ; Type
    dw 0    ; Flags. Optional
    dd .frame_buffer_end - .frame_buffer  ; Size
    dd 80   ; Width
    dd 25   ; Height
    dd 32   ; Depth idk

.frame_buffer_end:
    dw 0
    dw 0
    dd 5

.end:


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

    ; arg2
    push ebx

    ; arg1
    push eax

    ; Enter the kernel!
    ; kernel_main(arg1, arg2)
    extern kernel_main
    call kernel_main

    ; Kernel shouldn't return, but if it does, then loop forever
    cli
.hang:
    hlt
    jmp .hang
.end:


