
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

void main(void) {
    printf("%dhei%d", 69420, 99);
    printf("Shiish 0x%x\n", 20);
    printf("%dyuuh", 0);
    //putchar('a');
    //syscall(SYS_write, STDOUT_FILENO, "Hello world!\n", 13);
    //syscall(SYS_write, STDOUT_FILENO, "Hello world!\n", 13);

    for (;;) {}
}

