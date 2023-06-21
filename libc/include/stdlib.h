
#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((__noreturn__)) void abort(void);

char* itoa(int, char*, int);
char* uitoa(unsigned int, char*, int);

__attribute__((__noreturn__)) void exit(int);

#ifdef __cplusplus
}
#endif

#endif

