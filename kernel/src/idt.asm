
extern interrupt_handler
extern exception_handler


; void load_idt(idtr_t idtr);
;global load_idt
;load_idt:
;    lidt [esp + 6] ; Load IDT at stack-pointer - sizeof(idtr_t)
;    sti
;    ret

%macro isr_err_stub 1
isr_stub_%+%1:

    mov eax, 10
    mov ecx, 11
    mov edx, 12
    mov ebx, 13

    pushad

    push %1 ; interrupt id
    push 69
    push 420

    cld
    call exception_handler

    popad

    ;add esp, 4    ; restore esp (stack-pointer) to before interrupt id push
    ;add esp, 8    ; restore esp (stack-pointer) to before interrupt id push

    iret
%endmacro

%macro isr_no_err_stub 1
isr_stub_%+%1:

    pushad

    push %1 ; interrupt id
    push 69
    
    cld
    call interrupt_handler

    popad

    ;add esp, 4    ; restore esp (stack-pointer) to before interrupt id push
    ;add esp, 8    ; restore esp (stack-pointer) to before interrupt id push

    iret
%endmacro


; Define exception handlers
isr_no_err_stub 0
isr_no_err_stub 1
isr_no_err_stub 2
isr_no_err_stub 3
isr_no_err_stub 4
isr_no_err_stub 5
isr_no_err_stub 6
isr_no_err_stub 7
isr_err_stub    8
isr_no_err_stub 9
isr_err_stub    10
isr_err_stub    11
isr_err_stub    12
isr_err_stub    13
isr_err_stub    14
isr_no_err_stub 15
isr_no_err_stub 16
isr_err_stub    17
isr_no_err_stub 18
isr_no_err_stub 19
isr_no_err_stub 20
isr_no_err_stub 21
isr_no_err_stub 22
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_no_err_stub 29
isr_err_stub    30
isr_no_err_stub 31



global isr_stub_table
isr_stub_table:
%assign i 0 
%rep    32 
    dd isr_stub_%+i
%assign i i+1 
%endrep




