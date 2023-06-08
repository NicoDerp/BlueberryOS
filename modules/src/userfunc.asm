
_start:
    ;mov eax, 1    ; SYS_write
    ;mov ebx, 1    ; STDOUT_FILENO
    ;mov ecx, str  ; &str
    ;mov edx, 12   ; len(str)
    int 48        ; syscall
.loop:
    jmp .loop

str:
    db "Hello world!", 0

