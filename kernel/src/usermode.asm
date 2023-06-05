
global flush_tss
flush_tss:
    ;mov ax, (5 * 8) | 0
    mov ax, 28h
    ltr ax
    ret


global enter_usermode
enter_usermode:
    mov ax, (4 * 8) | 3
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, esp
    push (4 * 8) | 3
    push eax
    pushf
    push (3 * 8) | 3
    ;push test_user_function
    push 5000h
    iret

;test_user_function:
;    mov eax, 1
;    mov ebx, 1
;    mov ecx, str
;    mov edx, 13
;    int 48
;.loop:
;    jmp .loop


;str:
;    db "Hello world!", 0

