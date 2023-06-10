
global flush_tss
flush_tss:
    mov ax, (5 * 8) | 0
    ;mov ax, 28h
    ltr ax
    ret


global enter_usermode
enter_usermode:
    ; backup stack pointer for use
    mov ebp, esp
    cli

    mov ax, (4 * 8) | 3
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ;mov eax, esp
    push (4 * 8) | 3

    mov eax, [ebp+8]
    push eax

    pushf
    pop eax
    or eax, 0x200 ; set IF (enable interrupts)
    push eax

    push (3 * 8) | 3
    ;push test_user_function
    ;push 5000h
    ;push 400000h

    ; Why clear eax?
    xor eax, eax
    mov eax, [ebp+4]
    push eax

    ;push 0h
    ;push 20480
    ;push 0
    iret

;test_user_function:
;    mov eax, 1
;    mov ebx, 1
;    mov ecx, str
;    mov edx, 13
;    int 48
;.loop:
;    jmp .loop
;
;
;str:
;    db "Hello world!", 0

