
BITS 32

extern main

global _start
_start:
    call main

    mov eax, 0  ; SYS_exit
    mov ebx, 0  ; status 0
    int 30h     ; syscall

