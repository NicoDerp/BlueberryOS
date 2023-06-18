
BITS 32

section .text
global main
main:
    mov eax, 1    ; SYS_write
    mov ebx, 1    ; STDOUT_FILENO
    mov ecx, str  ; &str
    mov edx, 12   ; len(str)
    push eax
    int 30h       ; syscall
    pop eax
.loop:
    jmp .loop

section .data
str:
    db "Hello world!", 0

