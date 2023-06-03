
global enablePaging
enablePaging:
    mov eax, [esp+4]
    mov cr3, eax

    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax
    ret

;enablePaging:
;    push ebp
;    mov ebp, esp
;    mov eax, cr0
;    or eax, 0x80000000
;    mov cr0, eax
;    mov esp, ebp
;    pop ebp
;    ret

global loadPageDirectory
loadPageDirectory:
    push ebp
    mov eax, [esp+8]
    mov cr3, eax
    mov esp, ebp
    pop ebp
    ret

