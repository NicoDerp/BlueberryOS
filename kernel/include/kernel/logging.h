
#include <stdio.h>
#include <stdarg.h>



//#define _VERBOSE


#ifdef _VERBOSE

extern bool terminalInitialized;

#define VERBOSE(format, ...)\
    if (terminalInitialized) {\
        printf("[INFO] "format, ## __VA_ARGS__);\
    }\


#else


#define VERBOSE(format, ...)

void enableLogging(void);

#endif

