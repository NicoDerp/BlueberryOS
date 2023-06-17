
global enablePaging
enablePaging:
    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax
    ret

global flushPaging
flushPaging:
    mov eax, cr3
    mov cr3, eax
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
    mov eax, [esp+4]
    sub eax, 0xC0000000
    mov cr3, eax
    ret

