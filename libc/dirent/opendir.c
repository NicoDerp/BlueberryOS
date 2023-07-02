
#include <dirent.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <bits/types/struct_DIR.h>
#include <stdio.h>
#include <stdbool.h>


#define MAX_DIRECTORIES 1

extern int syscall2(int, int, int);
static DIR _directories[MAX_DIRECTORIES];

__attribute__((__nonnull__))
DIR* opendir(const char* name) {

    size_t i;
    bool found = false;
    for (i = 0; i < MAX_DIRECTORIES; i++) {
        if (!_directories[i].dd_used) {
            found = true;
            break;
        }
    }

    // Reached limit
    if (!found) {
        return (DIR*) 0;
    }

    int fd = syscall2(SYS_open, (int) name, O_RDONLY | O_DIRECTORY);
    if (fd == -1)
        return (DIR*) 0;

    DIR* d = &_directories[i];
    d->dd_size = 0;
    d->dd_loc = 0;
    d->dd_off = 0;
    d->dd_fd = fd;
    d->dd_used = true;

    return d;
}

