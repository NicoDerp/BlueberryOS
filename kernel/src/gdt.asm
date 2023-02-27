

gdtr:
    dw 0 ; limit storage
    dd 0 ; base storage
    

; void load_gdt(uint8_t limit, uint8 entry[8]);
global load_gdt
load_gdt:
    cli
    mov ax, [esp + 4]
    mov [gdtr], ax
    mov eax, [esp + 8]
    add eax, [esp + 12]
    mov [gdtr + 2], eax
    lgdt [gdtr]
    ret


; void reload_segments();
global reload_segments
reload_segments:
   ; Reload CS register containing code selector:
   jmp   0x08:.reload_cs ; 0x08 is a stand-in for your code segment
.reload_cs:
   ; Reload data segment registers:
   mov   ax, 0x10 ; 0x10 is a stand-in for your data segment
   mov   ds, ax
   mov   es, ax
   mov   fs, ax
   mov   gs, ax
   mov   ss, ax
   ret

