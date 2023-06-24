
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <stdlib.h>

void main(int a, int b) {
    for (size_t i = 0; i < 3; i++) {
        printf("Loop %d %d %d\n", i, a, b);
        //yield();
    }
    //putchar('a');
    //syscall(SYS_write, STDOUT_FILENO, "Hello world!\n", 13);
    //syscall(SYS_write, STDOUT_FILENO, "Hello world!\n", 13);

    //exit(0);
    //for (;;) {}
}

