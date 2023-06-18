
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

void main(void) {
    printf("Hei hei o");
    //putchar('a');
    //syscall(SYS_write, STDOUT_FILENO, "Hello world!\n", 13);
    //syscall(SYS_write, STDOUT_FILENO, "Hello world!\n", 13);

    for (;;) {}
}

