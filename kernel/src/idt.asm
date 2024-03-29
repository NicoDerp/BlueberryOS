
extern exception_handler
extern interrupt_handler
extern syscall_handler


; void load_idt(idtr_t idtr);
;global load_idt
;load_idt:
;    lidt [esp + 6] ; Load IDT at stack-pointer - sizeof(idtr_t)
;    sti
;    ret

%macro isr_exc_err_stub 1
isr_stub_%+%1:

    pushad

    push dword %1 ; interrupt id
    push dword 69
    push dword 420

    mov eax, cr2
    push eax

    cld
    call exception_handler

    add esp, 16     ; 'pop' interrupt-id, 2*test number and cr2

    popad

    add esp, 4      ; 'pop' error-code

    iret
%endmacro

%macro isr_exc_no_err_stub 1
isr_stub_%+%1:

    push dword 0  ; error-code
    
    pushad

    push dword %1 ; interrupt id
    push dword 69
    push dword 420

    mov eax, cr2
    push eax

    cld
    call exception_handler

    add esp, 16     ; 'pop' interrupt-id, 2*test number and cr2

    popad

    add esp, 4      ; 'pop' error-code

    iret
%endmacro

%macro irq_stub 1
isr_stub_%+%1:

    pushad

    push dword %1 ; interrupt id
    push dword 69
    push dword 420

    cld
    call interrupt_handler

    add esp, 12     ; 'pop' interrupt-id and 2*test-number

    popad

    iret
%endmacro

%macro syc_stub 1
isr_stub_%+%1:

    pushad

    push dword %1 ; interrupt id
    push dword 69
    push dword 420

    cld
    call syscall_handler

    add esp, 12     ; 'pop' interrupt-id and 2*test-number

    popad

    iret
%endmacro

; Define exception handlers
isr_exc_no_err_stub 0
isr_exc_no_err_stub 1
isr_exc_no_err_stub 2
isr_exc_no_err_stub 3
isr_exc_no_err_stub 4
isr_exc_no_err_stub 5
isr_exc_no_err_stub 6
isr_exc_no_err_stub 7
isr_exc_err_stub    8
isr_exc_no_err_stub 9
isr_exc_err_stub    10
isr_exc_err_stub    11
isr_exc_err_stub    12
isr_exc_err_stub    13
isr_exc_err_stub    14
isr_exc_no_err_stub 15
isr_exc_no_err_stub 16
isr_exc_err_stub    17
isr_exc_no_err_stub 18
isr_exc_no_err_stub 19
isr_exc_no_err_stub 20
isr_exc_no_err_stub 21
isr_exc_no_err_stub 22
isr_exc_no_err_stub 23
isr_exc_no_err_stub 24
isr_exc_no_err_stub 25
isr_exc_no_err_stub 26
isr_exc_no_err_stub 27
isr_exc_no_err_stub 28
isr_exc_no_err_stub 29
isr_exc_err_stub    30
isr_exc_no_err_stub 31

; ISR handlers
; PIC
irq_stub 32
irq_stub 33
irq_stub 34
irq_stub 35
irq_stub 36
irq_stub 37
irq_stub 38
irq_stub 39
irq_stub 40
irq_stub 41
irq_stub 42
irq_stub 43
irq_stub 44
irq_stub 45
irq_stub 46
irq_stub 47

; System call stuff
syc_stub 48

DESCRIPTOR_COUNT equ 49

global isr_stub_table
isr_stub_table:
%assign i 0
%rep    DESCRIPTOR_COUNT
    dd isr_stub_%+i
%assign i i+1
%endrep



