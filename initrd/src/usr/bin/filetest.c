
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>


void main(void) {

    int fd = open("hei", O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if (fd == -1) {
        int backup = errno;
        printf("open error: %s\n", strerror(backup));
        exit(1);
    }

    char* str = "Hei hei";
    if (write(fd, str, strlen(str)+1) == -1) {
        int backup = errno;
        printf("close error: %s\n", strerror(backup));
        exit(1);
    }

    if (close(fd) == -1) {
        int backup = errno;
        printf("close error: %s\n", strerror(backup));
        exit(1);
    }
}

