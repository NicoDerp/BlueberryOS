
#include <kernel/usermode.h>


tss_t sys_tss;

void tss_initialize(void) {
    // TODO set values
    sys_tss.ss0 = 0x10;

    sys_tss.iomap = (unsigned short) sizeof(tss_t);
}

void switch_to_usermode(void) {
    asm volatile("  \
        cli; \
        mov $0x23, %ax; \
        mov %ax, %ds; \
        mov %ax, %es; \
        mov %ax, %fs; \
        mov %ax, %gs; \
\
        mov %esp, %eax; \
        pushl $0x23; \
        pushl %eax; \
        pushf; \
        mov $0x200, %eax; \
        push %eax; \
        pushl $0x1B; \
        push $1f; \
        iret; \
        1: \
    "); 
}

