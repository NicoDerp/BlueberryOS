
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

global syscalla4
syscalla4:
    push edi
    push ebx
    push esi

    mov eax, [esp+4+3*4]
    mov edi, [esp+8+3*4]
    mov ebx, [edi+0]
    mov ecx, [edi+4]
    mov edx, [edi+8]
    mov esi, [edi+8]
    int 48

    pop esi
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

global syscall4
syscall4:
    push ebx
    push esi

    mov eax, [esp+12]
    mov ebx, [esp+16]
    mov ecx, [esp+20]
    mov edx, [esp+24]
    mov esi, [esp+28]
    int 48

    pop esi
    pop ebx
    ret

global syscall6
syscall6:
    push ebx
    push esi
    push edi
    push ebp

    mov eax, [esp+20]
    mov ebx, [esp+24]
    mov ecx, [esp+28]
    mov edx, [esp+32]
    mov esi, [esp+36]
    mov edi, [esp+40]
    mov ebp, [esp+44]
    int 48

    pop ebp
    pop edi
    pop esi
    pop ebx
    ret

