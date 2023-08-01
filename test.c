
#include <stdio.h>


int main() {

    int l = 0;
    for (unsigned int i = 0; i < 10; ++i) {
        printf("I: %d\n", i);
        if (i == 9 && !l) {
            l = 1;
            i = 0;
        }
    }

    return 0;
}

