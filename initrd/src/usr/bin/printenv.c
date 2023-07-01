
#include <stdio.h>
#include <stdlib.h>


void main(int argc, char* argv[]) {

    if (argc != 2) {
        printf("Usage: printenv VARIABLE\n");
        return;
    }

    char buf[256];
    if (getenv(argv[1], buf, sizeof(buf)) == 0) {
        printf("%s\n", buf);
    }
}

