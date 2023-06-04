
#include <stdio.h>
#include <stdint.h>
#include <string.h>

int main(void) {
    char* s1 = "Heic";
    char* s2 = "Heiac";
    printf("%d\n", strcmp(s1, s2));
}

