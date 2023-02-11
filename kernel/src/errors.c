
#include <kernel/errors.h>
#include <stdio.h>


void kerror(const char* msg) {
    printf("Kernel has run into an error: %s\n", msg);
    kabort();
}

