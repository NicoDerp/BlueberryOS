
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


int main(int argc, char* argv[]) {

    if (argc != 2)
        return 1;

    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if (fd == -1) {
        int backup = errno;
        printf("open error: %s\n", strerror(backup));
        return 1;
    }

    char* str = "Hei hei";
    if (write(fd, str, strlen(str)+1) == -1) {
        int backup = errno;
        printf("close error: %s\n", strerror(backup));
        return 1;
    }

    if (close(fd) == -1) {
        int backup = errno;
        printf("close error: %s\n", strerror(backup));
        return 1;
    }

    return 0;
}

