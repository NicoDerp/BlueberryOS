
BITS 32

section .text
global main
main:
    mov eax, 1    ; SYS_write
    mov ebx, 1    ; STDOUT_FILENO
    mov ecx, str  ; &str
    mov edx, 13   ; len(str)
    int 30h       ; syscall
    ret

section .data
str:
    db "Hello world!", 10, 0

