
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>



//#define _VERBOSE



#ifndef KERNEL_LOGGING_H
#define KERNEL_LOGGING_H



void enableLogging(void);


#ifdef _VERBOSE

extern bool loggingEnabled;

#define VERBOSE(format, ...)\
    if (loggingEnabled) {\
        printf("[INFO] "format, ## __VA_ARGS__);\
    }\

#else

#define VERBOSE(format, ...)

#endif



#endif


