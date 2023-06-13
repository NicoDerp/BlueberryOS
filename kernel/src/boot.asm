
extern kernelstart
extern kernelend

; Declare some constants for the multiboot header
MBALIGN       equ 1 << 0
MEMINFO       equ 1 << 1
MBFLAGS       equ MBALIGN | MEMINFO
MAGIC         equ 0xE85250D6
ARCHITECTURE  equ 0  ; 32-bit protected
HEADER_LENGTH equ multiboot_header.end - multiboot_header
CHECKSUM      equ -(MAGIC + ARCHITECTURE + HEADER_LENGTH)


KERNEL_PAGE_INDEX equ (0xC0000000 >> 22) ; 768

; Set the bytes
section .multiboot.data
align 4
multiboot_header:
    dd MAGIC
    dd ARCHITECTURE
    dd HEADER_LENGTH
    dd CHECKSUM

.frame_buffer:
    dw 5    ; Type
    dw 0    ; Flags. Optional
    dq .frame_buffer_end - .frame_buffer  ; Size
    dq 80   ; Width
    dq 25   ; Height
    dq 32   ; Depth idk

.frame_buffer_end:
    dw 0
    dw 0
    dq 5

.end:


; Reserve 16kb for stack
section .bootstrap_stack nobits
align 16
global stack_bottom
stack_bottom:
    resb 16384
stack_top:

section .bss
align 4096
page_directory:
    resb 4096
page_table1:
    resb 4096

; _start is the entry point to the kernel.
; Also say how big the function is.
section .multiboot.text
global _start:function (_start.end - _start)
_start:

    ; Now we are in 32-bit real-mode.

    ; Map first pagedirectory entry
    ; - Present    (1)
    ; - Read/write (1)
    ; - Kernel     (0)
    ; - Page-table is at page_table1
    mov ecx, (page_table1 - 0xC0000000 + 2)
    mov [page_directory - 0xC0000000 + 0], ecx

    ; Map higher-half
    ; - Present    (1)
    ; - Read/write (1)
    ; - Kernel     (0)
    ; - Page-table is at page_table1
    mov ecx, (page_table1 - 0xC0000000 + 2)
    mov [page_directory - 0xC0000000 + KERNEL_PAGE_INDEX*4], ecx

    ; Fill page table's pages
    mov edi, (page_table1 - 0xC0000000)
    mov esi, 0
    ;mov ecx, 1023
    mov ecx, 1024
.table1_1:
    cmp esi, kernelstart
    jl .table1_2

    ; If we have reached kernel_end then end loop
    cmp esi, (kernelend - 0xC0000000)
    jge .table1_3

    ; Write page entry which is at edi, and entry is esi | 2
    mov edx, esi
    or edx, 2
    mov [edi], edx

.table1_2:
    ; Page is 4096 bytes
    add esi, 4096

    ; Size of pagetable entry is 4 bytes (unsigned int)
    add edi, 4

    loop .table1_1

.table1_3:

    ; Map VGA memory to 0xC03FF000
    ; - Present
    ; - Read/write
    ;mov (page_table2 - 0xC0000000 + 1023 * 4), (0x000B8000 | 0x003)

    ; Tell the CPU where the page directory is
    mov eax, (page_directory - 0xC0000000)
    mov cr3, eax

    ; Enable paging
    mov ecx, cr0
    ;or ecx, 0x80000001
    or ecx, 0x80000000
    mov cr0, ecx

    ; Jump into higher-half!
    lea ecx, [HigherHalf]
    jmp ecx

.end:

section .text
HigherHalf:

    ; Unmap identity mapping since it is no longer needed
    mov [page_directory+0], dword 0

    ; Force a TLB flush
    mov ecx, cr3
    mov cr3, ecx

    ; Set the esp register to the top of the stack as it grows downwards.
    mov esp, stack_top

    ; arg2
    add ebx, 0xC0000000
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


