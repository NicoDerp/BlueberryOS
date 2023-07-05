
#ifndef _STRING_H
#define _STRING_H 1

#include <sys/cdefs.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int memcmp(const void*, const void*, size_t);
void* memcpy(void* __restrict, const void* __restrict, size_t);
void* memmove(void*, const void*, size_t);
void* memset(void*, int, size_t);

size_t strlen(const char*);

int strcmp(const char*, const char*);
int strncmp(const char*, const char*, size_t);

char* strcpy(char*, const char*);
char* strncpy(char*, const char*, size_t);

char* strtok(char*, const char*);
char* strchr(const char*, int);

char* strerror(int errnum);

#ifdef __cplusplus
}
#endif

#endif /* _STRING_H */

