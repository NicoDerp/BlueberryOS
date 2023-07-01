
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>


void main(int argc, char** argv) {

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

        exit(0);
    }

    if (argc != 2) {
        printf("Usage: cat FILE\n");
        exit(1);
    }

    FILE* fp = open(argv[1], O_RDONLY);
    if (fd == -1) {
        printf("%s: %s: No such file or directory\n", argv[0], argv[1]);
        exit(1);
    }

    char buf[128];
    int bytes = read(fd, buf, sizeof(buf));
    if (bytes != 0) {
        buf[bytes] = '\0';
        printf("%s", buf);
    }

    if (close(fd) == -1) {
        printf("close failed\n");
        exit(1);
    }
}

