
global syscall0
syscall0:
    mov eax, [esp+4]
    int 48
    ret

global syscall1
syscall1:
    mov eax, [esp+4]
    mov ebx, [esp+8]
    int 48
    ret

global syscall2
syscall2:
    mov eax, [esp+4]
    mov ebx, [esp+8]
    mov ecx, [esp+12]
    int 48
    ret

global syscall3
syscall3:
    mov eax, [esp+4]
    mov ebx, [esp+8]
    mov ecx, [esp+12]
    mov edx, [esp+16]
    int 48
    ret


