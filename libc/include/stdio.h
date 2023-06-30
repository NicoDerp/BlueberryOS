
#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>

#define stdin  0
#define stdout 1
#define stderr 2

#define EOF   (-1)

#ifdef __cplusplus
extern "C" {
#endif

//int printf(const char* __restrict, ...) __attribute__(( format( printf, 1, 2 ) ));
int printf(const char* __restrict, ...);
int putchar(int);
int puts(const char*);

int getc(unsigned int);
int getchar(void);

#ifdef __cplusplus
}
#endif

#endif

