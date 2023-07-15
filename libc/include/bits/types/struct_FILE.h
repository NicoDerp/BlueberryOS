
#ifndef _struct_FILE_H
#define _struct_FILE_H 1


struct _IO_FILE
{
    char* dd_buf;
    unsigned int dd_size;
    unsigned int dd_index;
    int dd_fd;
    int dd_used;
};

#endif

