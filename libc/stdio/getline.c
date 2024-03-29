
#include <stdio.h>
#include <bits/types/struct_FILE.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <bits/memory.h>

ssize_t getline(char** __restrict lineptr, size_t* __restrict n, FILE* __restrict fp) {

    int total = 0;
    int bytes;

    if (lineptr == NULL || n == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (fp->dd_buf == NULL) {
        fp->dd_size = 512;
        fp->dd_buf = (char*) malloc(512);

        bytes = read(fp->dd_fd, fp->dd_buf, 511);
        if (bytes <= 0)
            return -1;

        total += bytes;
        fp->dd_index += bytes;
        fp->dd_buf[bytes] = '\0';
    }

    if (fp->dd_index <= 0) {

        bytes = read(fp->dd_fd, fp->dd_buf, fp->dd_size-1);
        if (bytes <= 0)
            return -1;

        total += bytes;
        fp->dd_index += bytes;
        fp->dd_buf[bytes] = '\0';
    }

    char* pos;
    int hasRead = 0;
    while ((pos = strchr(fp->dd_buf, '\n')) == NULL) {
        hasRead = 1;

        if (fp->dd_index + 512 > fp->dd_size) {
            fp->dd_buf = realloc(fp->dd_buf, fp->dd_index + 512);
            fp->dd_size = fp->dd_index + 512;
        }

        bytes = read(fp->dd_fd, fp->dd_buf + fp->dd_index, 511);
        //if (bytes <= 0)
        if (bytes < 0)
            return -1;

        fp->dd_buf[fp->dd_index + bytes] = '\0';
        fp->dd_index += bytes;
        total += bytes;

        // EOF
        if (bytes == 0) {
            pos = &fp->dd_buf[fp->dd_index];
            break;
        }
    }

    // No bytes read so EOF
    if (total == 0 && hasRead)
        return -1;

    unsigned int linesize = (unsigned int) pos - (unsigned int) fp->dd_buf;

    if (*lineptr == NULL) {
        *lineptr = (char*) malloc(linesize+1);
    }
    else if (linesize+1 != *n) {
        *lineptr = (char*) realloc(*lineptr, linesize+1);
    }

    *n = linesize + 1;

    memcpy(*lineptr, fp->dd_buf, linesize);
    (*lineptr)[linesize] = '\0';

    memmove(fp->dd_buf, pos + 1, fp->dd_size - linesize - 1);
    fp->dd_index -= linesize + 1;

    return linesize;
}

