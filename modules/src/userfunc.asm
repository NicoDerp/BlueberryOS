
userfunc:
    mov eax, 1
    mov ebx, 1
    mov ecx, str
    mov edx, 13
    int 48
.loop:
    jmp .loop


str:
    db "Hello world!", 0

