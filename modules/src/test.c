
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <stdlib.h>

void main(void) {
    for (size_t i = 0; i < 3; i++) {
        printf("Loop %d\n", i);
        //yield();
    }
    //putchar('a');
    //syscall(SYS_write, STDOUT_FILENO, "Hello world!\n", 13);
    //syscall(SYS_write, STDOUT_FILENO, "Hello world!\n", 13);

    //exit(0);
    //for (;;) {}
}

