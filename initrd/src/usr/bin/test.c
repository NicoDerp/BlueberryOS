
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>


int main(int argc, char* argv[]) {

    printf("argc: %d\n", argc);

    for (int i = 0; i < argc; i++) {
        printf("arg %d: '%s'\n", i, argv[i]);
    }

    return 0;
}

