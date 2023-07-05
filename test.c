
#include <stdio.h>


extern void myfunc(void);

int errint;

int main() {

    errint = 0;
    printf("errno %d\n", errint);
    myfunc();
    printf("errno %d\n", errint);

    return 0;
}

