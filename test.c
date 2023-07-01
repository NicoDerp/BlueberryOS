
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

int main ()
{

    FILE* fp = fopen("test.c", "r");
    if (!fp) {
        printf("error openining file\n");
        return 1;
    }

    fclose(fp);

    return 0;
}

