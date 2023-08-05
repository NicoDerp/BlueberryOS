
#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/types.h>
#include <sys/cdefs.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

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
int vprintf(const char* __restrict, va_list);
int printf(const char* __restrict, ...);
int fprintf(FILE*, const char* __restrict, ...);
int putchar(int);
int puts(const char*);

int getc(unsigned int);
int getchar(void);

FILE* fopen(const char*, const char*);
FILE* fdopen(int fd, const char* mode);
int fclose(FILE* file);
//char* fgets(char* str, int n, FILE* stream);

int fflush(FILE* stream);

ssize_t getline(char** __restrict, size_t* __restrict, FILE* __restrict);


#ifdef __cplusplus
}
#endif

#endif

