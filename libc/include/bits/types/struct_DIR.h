
#ifndef _struct_DIRECTORY_H
#define _struct_DIRECTORY_H 1

#include <dirent.h>
#include <stdint.h>

struct __dirstream
{
    struct dirent dd_buf[4];
    uint32_t dd_loc;
    uint32_t dd_size;
    uint32_t dd_fd;
    uint8_t dd_used;
};

#endif

