
#include <stdio.h>
#include <unistd.h>


void main(int argc, char* argv[]) {
    printf("uid: %d\n", getuid());
    setuid(0);
    printf("uid: %d\n", getuid());
}

