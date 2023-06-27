
global syscalla0
syscalla0:
    mov eax, [esp+4]
    int 48
    ret

global syscalla1
syscalla1:
    push edi
    push ebx

    mov eax, [esp+4+2*4]
    mov edi, [esp+8+2*4]
    mov ebx, [edi]
    int 48

    pop ebx
    pop edi
    ret

global syscalla2
syscalla2:
    push edi
    push ebx

    mov eax, [esp+4+2*4]
    mov edi, [esp+8+2*4]
    mov ebx, [edi+0]
    mov ecx, [edi+4]
    int 48

    pop ebx
    pop edi
    ret

global syscalla3
syscalla3:
    push edi
    push ebx
    ;push edi

    mov eax, [esp+4+2*4]
    mov edi, [esp+8+2*4]
    mov ebx, [edi+0]
    mov ecx, [edi+4]
    mov edx, [edi+8]
    int 48

    ;pop edi
    pop ebx
    pop edi
    ret








global syscall0
syscall0:
    mov eax, [esp+4]
    int 48
    ret

global syscall1
syscall1:
    push ebx

    mov eax, [esp+8]
    mov ebx, [esp+12]
    int 48

    pop ebx
    ret

global syscall2
syscall2:
    push ebx

    mov eax, [esp+8]
    mov ebx, [esp+12]
    mov ecx, [esp+16]
    int 48

    pop ebx
    ret

global syscall3
syscall3:
    push ebx
    ;push edi

    mov eax, [esp+8]
    mov ebx, [esp+12]
    mov ecx, [esp+16]
    mov edx, [esp+20]
    int 48

    ;pop edi
    pop ebx
    ret


