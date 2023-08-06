
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char* argv[]) {

    if (argc != 2) {
        printf("Usage: printenv VARIABLE\n");
        return 1;
    }

    char buf[256];
    if (getenv_r(argv[1], buf, sizeof(buf)) == 0) {
        printf("%s\n", buf);
    }

    return 0;
}

