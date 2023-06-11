
BITS 32

section .text
main:
    mov eax, 1    ; SYS_write
    mov ebx, 1    ; STDOUT_FILENO
    mov ecx, str  ; &str
    mov edx, 12   ; len(str)
    int 30h       ; syscall
.loop:
    jmp .loop

;section .data
str:
    db "Hello world!", 0

