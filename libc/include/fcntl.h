
#ifndef _FCNTL_H
#define _FCNTL_H 1

#include <stdarg.h>


#define O_RDONLY    1
#define O_WRONLY    2
#define O_RDWR      (O_RDONLY | O_WRONLY)
#define O_DIRECTORY 4
#define O_TRUNC     8
#define O_CREAT     16

#ifdef __cplusplus
extern "C" {
#endif

int open(const char*, int, ...);

#ifdef __cplusplus
}
#endif
#endif

