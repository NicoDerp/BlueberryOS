
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>


void main(void) {

    char* filename = "/usr/bin/echo";

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        int backup = errno;

        printf("open error: %s\n", strerror(backup));
        exit(1);
    }

    FILE* fp = fdopen(fd, "r");

    char* line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, fp)) != -1) {

        while (line[linelen-1] == '\n')
            line[--linelen] = '\0';

        printf("Appending %ld:'%s'\n", linelen, line);
        getchar();
    }

    free(line);

    if (fclose(fp) == -1) {
        int backup = errno;
        printf("fclose error: %s\n", strerror(backup));
        exit(1);
    }

}

