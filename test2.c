
#include <stdio.h>

extern void test(int a, int b, int c, int d, int e, int f, int g);

int main(void) {
    test(5, 4, 7, 1, 2, 3, 5);
    return 0;
}

