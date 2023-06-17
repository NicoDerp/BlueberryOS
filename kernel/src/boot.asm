
extern _kernelstart
extern _kernelend

; Declare some constants for the multiboot header
MAGIC         equ 0xE85250D6
ARCHITECTURE  equ 0  ; 32-bit protected
HEADER_LENGTH equ multiboot_header.end - multiboot_header
CHECKSUM      equ -(MAGIC + ARCHITECTURE + HEADER_LENGTH)


KERNEL_PAGE_INDEX equ (0xC0000000 >> 22) ; 768

; Set the bytes
section .multiboot.data align=4096
;section .multiboot.data
;align 4
multiboot_header:
    dd MAGIC
    dd ARCHITECTURE
    dd HEADER_LENGTH
    dd CHECKSUM

;align 8
.tag_info_start:
    dw 1
    dw 1
    dd .tag_info_end - .tag_info_start

    dd 5
.tag_info_end:
    dd 0
    dd 0
    dd 8

;align 8
.tag_frame_buffer_start:
    dw 5    ; Type (framebuffer)
    dw 0    ; Flags. Optional
    ;dw 1    ; Flags. Optional
    dd .tag_frame_buffer_end - .tag_frame_buffer_start  ; Size
    dd 80   ; Width
    dd 25   ; Height
    dd 0   ; Depth. Zero in text mode
.tag_frame_buffer_end:
    dd 0
    dd 0
    dd 8
;    dw 0
;    dw 0
;    dd 5

align 8
.tag_end_start:
    dw 0
    dw 0
    dd .tag_end_end - .tag_end_start
.tag_end_end:

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
global page_directory
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
    mov ecx, (page_table1 - 0xC0000000 + 3)
    mov [page_directory - 0xC0000000 + 0], ecx

    ; Map higher-half
    mov [page_directory - 0xC0000000 + KERNEL_PAGE_INDEX*4], ecx

    ; Fill page table's pages
    mov edi, (page_table1 - 0xC0000000)
    mov esi, 0
    mov ecx, 1023
    ;mov ecx, 1024
.table1_1:
    cmp esi, _kernelstart
    jl .table1_2

    ; If we have reached kernel_end then end loop
    ;cmp esi, (_kernelend - 0xC0000000)
    cmp esi, (0xFFFFFFFF - 0xC0000000)
    jge .table1_3

    ; Write page entry which is at edi, and entry is esi | 3 which is
    ; read/write | present
    mov edx, esi
    or edx, 3
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
    mov [page_table1 - 0xC0000000 + 1023 * 4], dword (0x000B8000 | 0x003)

    ; Tell the CPU where the page directory is
    mov ecx, (page_directory - 0xC0000000)
    mov cr3, ecx

    ; Enable paging
    mov ecx, cr0
    ;or ecx, 0x80010000
    or ecx, 0x80000001
    ;or ecx, 0x80000000
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
    ;mov ecx, cr3
    ;mov cr3, ecx

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


