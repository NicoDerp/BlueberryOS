
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <bits/types/struct_FILE.h>
#include <fcntl.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>


#define MAX_FILES 8

extern int syscall2(int, int, int);
static FILE _files[MAX_FILES];

FILE* fopen(const char* filename, const char* mode) {

    size_t i;
    bool found = false;
    for (i = 0; i < MAX_FILES; i++) {
        if (!_files[i].dd_used) {
            found = true;
            break;
        }
    }

    // Reached limit
    if (!found) {
        return (FILE*) 0;
    }

    int flags;
    if (strcmp(mode, "r") == 0) {
        flags = O_RDONLY;
    }
    else if (strcmp(mode, "w") == 0) {
        flags = O_WRONLY;
    }
    else if (strcmp(mode, "r+") == 0) {
        flags = O_RDWR;
    }
    // Invalid mode
    else {
        return (FILE*) 0;
    }

    int fd = syscall2(SYS_open, (int) filename, flags);
    if (!fd)
        return (FILE*) 0;

    FILE* f = &_files[i];
    f->dd_fd = fd;
    f->dd_used = true;

    return f;
}

