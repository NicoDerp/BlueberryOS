
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>



//#define _VERBOSE



#ifndef KERNEL_LOGGING_H
#define KERNEL_LOGGING_H


extern bool loggingEnabled;

void enableLogging(void);

#define ERROR(format, ...)\
    if (loggingEnabled) {\
        printf("\e[4;0m[ERROR]\e[0m "format, ## __VA_ARGS__);\
    }\


#define FATAL(format, ...)\
    if (loggingEnabled) {\
        printf("\e[4;0m[FATAL]\e[0m "format, ## __VA_ARGS__);\
    }\


#ifdef _VERBOSE

#define VERBOSE(format, ...)\
    if (loggingEnabled) {\
        printf("\e[f;0m[INFO]\e[0m "format, ## __VA_ARGS__);\
    }\

#else
#define VERBOSE(format, ...)
#endif

#endif


