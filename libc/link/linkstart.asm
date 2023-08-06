
extern main

section .text
global _start:function (_start.end - _start)

    call main

    mov ebx, eax  ; status
    mov eax, 0    ; SYS_exit
    int 30h       ; syscall


