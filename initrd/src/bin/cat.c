
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


int main(int argc, char** argv) {

    if (argc == 1) {

        while (1) {
            char buf[1024];
            size_t i = 0;
            char c;
            while ((c = getchar()) != '\n') {
                buf[i++] = c;
                putchar(c);
            }
            buf[i] = '\0';
            putchar('\n');

            if (strcmp(buf, "EOF") == 0)
                break;

            printf("%s\n", buf);
        }

        return 0;
    }

    if (argc != 2) {
        printf("Usage: cat FILE\n");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        int backup = errno;
        printf("%s: %s: %s\n", argv[0], argv[1], strerror(backup));
        return 1;
    }

    struct stat st;
    if (stat(argv[1], &st) == -1) {
        int backup = errno;
        printf("%s: %s: stat error: %s\n", argv[0], argv[1], strerror(backup));
        return 1;
    }

    // TODO mmap
    char* buf = (char*) malloc(st.st_size + 1);
    int bytes = read(fd, buf, st.st_size);
    if (bytes != 0) {
        buf[bytes] = '\0';
        printf("%s", buf);
    }
    free(buf);

    if (close(fd) == -1) {
        printf("close failed\n");
        return 1;
    }

    return 0;
}

