
BITS 64

extern errint

global myfunc
myfunc:
    ;mov rax, 69
    ;mov [errint], rax
    inc qword [errint]
    ret


