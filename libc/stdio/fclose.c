
#include <stdio.h>
#include <sys/syscall.h>
#include <bits/types/struct_FILE.h>
#include <stdbool.h>
#include <stdlib.h>


extern int syscall1(int, int);

int fclose(FILE* file) {
    int status = syscall1(SYS_close, file->dd_fd);

    if (file->dd_buf)
        free(file->dd_buf);

    file->dd_used = false;

    return status;
}

