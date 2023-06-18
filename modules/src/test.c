
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

void main(void) {
    printf("%dhei%d", 69420, 99);
    //putchar('a');
    //syscall(SYS_write, STDOUT_FILENO, "Hello world!\n", 13);
    //syscall(SYS_write, STDOUT_FILENO, "Hello world!\n", 13);

    for (;;) {}
}

