
extern __errno

global syscalla0
syscalla0:
    mov eax, [esp+4]
    int 48

    test ecx, ecx
    jz .skip
    mov [__errno], ecx
.skip:
    ret

global syscalla1
syscalla1:
    push edi
    push ebx

    mov eax, [esp+4+2*4]
    mov edi, [esp+8+2*4]
    mov ebx, [edi]
    int 48

    test ecx, ecx
    jz .skip
    mov [__errno], ecx
.skip:

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

    test ecx, ecx
    jz .skip
    mov [__errno], ecx
.skip:

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

    test ecx, ecx
    jz .skip
    mov [__errno], ecx
.skip:

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
    mov esi, [edi+12]
    int 48

    test ecx, ecx
    jz .skip
    mov [__errno], ecx
.skip:

    pop esi
    pop ebx
    pop edi
    ret

global syscalla5
syscalla5:
    push ebx
    push esi
    push edi

    mov eax, [esp+4+3*4]
    mov edi, [esp+8+3*4]
    mov ebx, [edi+0]
    mov ecx, [edi+4]
    mov edx, [edi+8]
    mov esi, [edi+12]
    mov edi, [edi+16]
    int 48

    test ecx, ecx
    jz .skip
    mov [__errno], ecx
.skip:

    pop edi
    pop esi
    pop ebx
    ret

global syscalla6
syscalla6:
    push ebx
    push esi
    push edi
    push ebp

    mov eax, [esp+4+4*4]
    mov edi, [esp+8+4*4]
    mov ebx, [edi+0]
    mov ecx, [edi+4]
    mov edx, [edi+8]
    mov esi, [edi+12]
    mov ebp, [edi+20]
    mov edi, [edi+16]
    int 48

    test ecx, ecx
    jz .skip
    mov [__errno], ecx
.skip:

    pop ebp
    pop edi
    pop esi
    pop ebx
    ret






global syscall0
syscall0:
    mov eax, [esp+4]
    int 48

    test ecx, ecx
    jz .skip
    mov [__errno], ecx
.skip:

    ret

global syscall1
syscall1:
    push ebx

    mov eax, [esp+8]
    mov ebx, [esp+12]
    int 48

    test ecx, ecx
    jz .skip
    mov [__errno], ecx
.skip:

    pop ebx
    ret

global syscall2
syscall2:
    push ebx

    mov eax, [esp+8]
    mov ebx, [esp+12]
    mov ecx, [esp+16]
    int 48

    test ecx, ecx
    jz .skip
    mov [__errno], ecx
.skip:

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

    test ecx, ecx
    jz .skip
    mov [__errno], ecx
.skip:

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

    test ecx, ecx
    jz .skip
    mov [__errno], ecx
.skip:

    pop esi
    pop ebx
    ret

global syscall5
syscall5:
    push ebx
    push esi
    push edi

    mov eax, [esp+16]
    mov ebx, [esp+20]
    mov ecx, [esp+24]
    mov edx, [esp+28]
    mov esi, [esp+32]
    mov edi, [esp+36]
    int 48

    test ecx, ecx
    jz .skip
    mov [__errno], ecx
.skip:

    pop edi
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

    test ecx, ecx
    jz .skip
    mov [__errno], ecx
.skip:

    pop ebp
    pop edi
    pop esi
    pop ebx
    ret

