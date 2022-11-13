
; Make the assembly code know these labels exist.
extern kernel_main

; Declare constants for the multiboot header.
MBALIGN  equ   1 << 0              ; Align loaded modules on page boundaries
MEMINFO  equ   1 << 1              ; Provide memory map
;FLAGS    equ   MBALIGN | MEMINFO   ; This is the Multiboot 'flag' field
FLAGS    equ   0x00000007
;MAGIC    equ   0x1BADB002          ; 'Magic number' lets bootloader find the header
MAGIC    equ   0xE85250D6
CHECKSUM equ -(MAGIC + FLAGS)     ; Checksum of above, to prove we are multiboot


; Declare a multiboot header that marks the program as a kernel. These are magic values
; that are documented in the multiboot standard. The bootloader will search for
; these signatures in the first 8kb of the kernel file, aligned at a 32-bit boundry.
; The signature is in it's own section so the header is forced to be within
; the first 8kb of the kernel.

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

    dd 0
    dd 0
    dd 0
    dd 0
    dd 0

    dd 0
    dd 1024
    dd 768
    dd 32


; It is up to the kernel to provide a stack. So we need to set the stack pointer register (esp).
; We allocate a small stack by creating a symbol at the top and bottom. We have 16kb (16384 bytes).
; The stack grows downwards on x86. The stack is in its own section so we can mark it as nobits.
; Meaning it is unititialized, so in the kernel file. The stack on x86 must be 16-byte aligned.
; The compiler will assume the stack is properly aligned and failure to do so will result
; in undefined behaviour.

section .bss
align 16

stack_bottom:
    resb 16384  ; 16kb

stack_top:


; The linker script specifies _start as then entry point to the kernel and the bootloader
; will jump to this position once the kernel has been loaded. It doesn't make sense to return
; from this function as the bootloader is gone. Declare _start as a function symbol with
; the given symbol size.

section .text

global _start:function (_start.end - _start)
_start:
    ; The bootloader has loaded us into 32-bit protected mode on a x86 machine.
    ; Interrupts are disabled. Paging is disabled. The processor state is defined in the
    ; multiboot standard. The kernel has full control over the CPU. The kernel can only
    ; make use of hardware features and any code it provides as part of itself.
    ; There's no printf function if the kernel doesn't implement it itself.
    ; There are no security restrictions, no safeguards, no debugging mechanisms,
    ; only what the kernel provides. It has the absolute and complete power over the machine.

    ; To set up the stack we must set the stack pointer register to the stack top.

    mov esp, stack_top

    ; This is a good place to initialize crucial stuff before the kernel is entered.
    ; It's best to minimize the early enviroment where crucial features are offline.
    ; Note that the processor is not fully initialized yet. Features such as floating point
    ; instructions and instruction set extensions are not initialized yet. The GDT should
    ; be loaded here. Paging should be enabled here.

    ; Enter the high-level kernel.

    ; Push ebx which contains a pointer to Multiboot information structure
    push ebx

    call kernel_main

    ; If the system has nothing more to do, put the computer in an infinite loop.

    cli              ; Disable interrupts just in case
.loop:
    hlt              ; Halt the CPU by waiting for the next interrupt (they are disabled).
                     ; This will lock the computer
    jmp .loop        ; Jump back and repeat

.end:







