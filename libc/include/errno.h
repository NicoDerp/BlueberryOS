
#ifndef _ERRNO_H
#define _ERRNO_H 1

#include <sys/cdefs.h>
#include <asm-generic/errno-values.h>

extern int __errno;
#define errno __errno
//#define errno (*(__errno_location()))
//extern int* __errno_location(void) __attribute__((const));

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif

