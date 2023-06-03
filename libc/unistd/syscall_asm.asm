
global syscall0
syscall0:
    mov eax, [esp+4]
    int 48
    ret

global syscall1
syscall1:
    mov eax, [esp+4]
    mov edi, [esp+8]
    mov ebx, [edi]
    int 48
    ret

global syscall2
syscall2:
    mov eax, [esp+4]
    mov edi, [esp+8]
    mov ebx, [edi+0]
    mov ecx, [edi+4]
    int 48
    ret

global syscall3
syscall3:
    mov eax, [esp+4]
    mov edi, [esp+8]
    mov ebx, [edi+0]
    mov ecx, [edi+4]
    mov edx, [edi+8]
    int 48
    ret


