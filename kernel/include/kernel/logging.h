
#include <stdio.h>
#include <stdarg.h>



#define _VERBOSE


#ifdef _VERBOSE

#define VERBOSE(format, ...)\
    printf("[INFO] "format, ## __VA_ARGS__);

#else

#define VERBOSE(format, ...)

#endif

