
#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>
#include <stdint.h>

#define stdin  0
#define stdout 1
#define stderr 2

#define EOF   (-1)


struct _IO_FILE;
typedef struct _IO_FILE FILE;

#ifdef __cplusplus
extern "C" {
#endif

//int printf(const char* __restrict, ...) __attribute__(( format( printf, 1, 2 ) ));
int printf(const char* __restrict, ...);
int putchar(int);
int puts(const char*);

int getc(unsigned int);
int getchar(void);

FILE* fopen(const char*, const char*);
int fclose(FILE* file);
//char* fgets(char* str, int n, FILE* stream);

#ifdef __cplusplus
}
#endif

#endif

