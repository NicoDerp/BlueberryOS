
global flush_tss
flush_tss:
    mov ax, (5 * 8) | 0
    ;mov ax, 28h
    ltr ax
    ret


; enter_usermode(code_addr, new_stack_pointer, registers)
global enter_usermode
enter_usermode:
    ; backup stack pointer for use
    mov ecx, esp
    cli

    mov ax, (4 * 8) | 3
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ;mov eax, esp
    push (4 * 8) | 3

    ; Stack pointer
    mov eax, [ecx+8]
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
    ; Code address
    xor eax, eax
    mov eax, [ecx+4]
    push eax

    mov eax, [ecx+12]
    mov ebx, [ecx+16]
    mov edx, [ecx+20]
    mov ebp, [ecx+24]
    mov esi, [ecx+28]
    mov edi, [ecx+32]
    mov ecx, [ecx+36]

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

