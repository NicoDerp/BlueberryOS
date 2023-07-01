
#include <stdio.h>
#include <unistd.h>


int main() {

    char path[512];
    if (getcwd(path, sizeof(path)) == NULL) {
        printf("error, getcwd failed\n");
        return -1;
    }

    printf("%s\n", path);

    return 0;
}



