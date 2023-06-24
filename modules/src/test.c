
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>

void main(int argc, char** argv) {

    (void) argv;
    printf("argc: %d\n", argc);

    /*
    for (int i = 0; i < argc; i++) {
        printf("arg %d: %d\n", i, argv[0]);
    }
    */
    printf("arg 0: 0x%x\n", argv);

}

