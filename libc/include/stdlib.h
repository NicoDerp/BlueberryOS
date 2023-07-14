
#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <sys/cdefs.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((__noreturn__)) void abort(void);

char* itoa(int, char*, int);
char* uitoa(unsigned int, char*, int);
int atoi(const char* str);

__attribute__((__noreturn__)) void exit(int);

int getenv_r(const char*, char*, size_t);
int setenv(const char*, const char*, int);
int unsetenv(const char*);

void* malloc(size_t);
void free(void*);
void* realloc(void*, size_t);

#ifdef __cplusplus
}
#endif

#endif

